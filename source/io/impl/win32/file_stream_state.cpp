#include "concurrencpp/io/constants.h"
#include "concurrencpp/results/resume_on.h"
#include "concurrencpp/io/impl/win32/utils.h"
#include "concurrencpp/io/impl/win32/io_engine.h"
#include "concurrencpp/io/impl/win32/io_awaitable.h"
#include "concurrencpp/io/impl/common/io_cancelled.h"
#include "concurrencpp/io/impl/win32/file_stream_state.h"

#include <cassert>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using concurrencpp::lazy_result;
using concurrencpp::io_engine;
using concurrencpp::details::win32::file_stream_state;

file_stream_state::file_stream_state(std::filesystem::path path, file_open_mode open_mode, std::shared_ptr<io_engine> engine) noexcept
    :
    io_object(open_file_handle(path, open_mode), close_file_handle, std::move(engine)),
    m_path(std::move(path)), m_open_mode(open_mode) {

    if (((open_mode & file_open_mode::append) == file_open_mode::append) ||
        ((open_mode & file_open_mode::at_end) == file_open_mode::at_end)) {
        m_write_pos = static_cast<size_t>(-1);  // Start at the end of the file.
    }
}

void* file_stream_state::open_file_handle(const std::filesystem::path& path, file_open_mode mode) {
    const auto o_mode = static_cast<int>(mode);

    DWORD desired_access = 0x0;
    if (o_mode & static_cast<int>(file_open_mode::read)) {
        desired_access |= GENERIC_READ;
    }
    if (o_mode & static_cast<int>(file_open_mode::write)) {
        desired_access |= GENERIC_WRITE;
    }

    DWORD creation_disposition = 0x0;
    if (o_mode & static_cast<int>(file_open_mode::read)) {
        if (o_mode & static_cast<int>(file_open_mode::write)) {
            creation_disposition = OPEN_ALWAYS;
        } else {
            creation_disposition = OPEN_EXISTING;
        }
    } else if (o_mode & static_cast<int>(file_open_mode::truncate)) {
        creation_disposition = CREATE_ALWAYS;
    } else {
        creation_disposition = OPEN_ALWAYS;
    }

    DWORD share_mode = 0x0;
    if (o_mode & static_cast<int>(file_open_mode::read)) {
        share_mode |= FILE_SHARE_READ;
    }
    if (o_mode & static_cast<int>(file_open_mode::write)) {
        share_mode |= FILE_SHARE_WRITE;
    }

    const auto file_handle =
        ::CreateFileW(path.c_str(), desired_access, share_mode, nullptr, creation_disposition, FILE_FLAG_OVERLAPPED, nullptr);

    if (file_handle == INVALID_HANDLE_VALUE) {
        throw_system_error(::GetLastError());
    }

    return file_handle;
}

void file_stream_state::close_file_handle(void* handle) noexcept {
    ::CancelIoEx(handle, nullptr);
    const auto res = ::CloseHandle(handle);
    assert(res);
}

std::filesystem::path file_stream_state::get_path() const noexcept {
    return m_path;
}

concurrencpp::file_open_mode file_stream_state::get_open_mode() const noexcept {
    return m_open_mode;
}

lazy_result<bool> file_stream_state::eof_reached(std::shared_ptr<file_stream_state> self,
                                                 std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self));
    assert(static_cast<bool>(resume_executor));
    auto sg = co_await self->m_lock.lock(resume_executor);
    co_return self->m_eof_reached;
}

lazy_result<size_t> file_stream_state::read(std::shared_ptr<file_stream_state> self_ptr,
                                            std::shared_ptr<concurrencpp::executor> resume_executor,
                                            std::shared_ptr<io_engine> io_engine,
                                            void* buffer,
                                            size_t buffer_length,
                                            std::stop_token* optional_stop_token) {

    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

    if (buffer_length == 0) {
        co_return 0;
    }

    DWORD length = 0;
    if (buffer_length > std::numeric_limits<DWORD>::max()) {
        length = std::numeric_limits<DWORD>::max();
    } else {
        length = static_cast<DWORD>(buffer_length);
    }

    // todo: handle cancellation
    auto sg = co_await self.m_lock.lock(resume_executor);
    if (self.m_eof_reached) {
        co_return 0;
    }

    uint32_t read, ec;

    {
        std::tie(read, ec) = co_await read_awaitable(self_ptr,
                                                     *io_engine,
                                                     std::move(resume_executor),
                                                     buffer,
                                                     length,
                                                     self.m_read_pos,
                                                     optional_stop_token);
    }

    // Handles EOF - eof is not an error, in this case correct the result to be bytes and mark eof
    if (ec != 0) {
        if (ec == ERROR_HANDLE_EOF) {
            self.m_eof_reached = true;
            co_return 0;
        }

        if (ec == ERROR_OPERATION_ABORTED) {
            throw io_cancelled(details::consts::k_file_stream_read_io_cancelled_err_msg);
        }

        throw_system_error(ec);
    }

    self.m_read_pos += read;
    co_return read;
}

lazy_result<size_t> file_stream_state::write(std::shared_ptr<file_stream_state> self_ptr,
                                             std::shared_ptr<concurrencpp::executor> resume_executor,
                                             std::shared_ptr<io_engine> io_engine,
                                             const void* buffer,
                                             size_t buffer_length,
                                             std::stop_token* optional_stop_token) {

    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

    if (buffer_length == 0) {
        co_return 0;
    }

    DWORD length = 0;
    if (buffer_length > std::numeric_limits<DWORD>::max()) {
        length = std::numeric_limits<DWORD>::max();
    } else {
        length = static_cast<DWORD>(buffer_length);
    }

    // todo: handle cancellation
    auto sg = co_await self.m_lock.lock(resume_executor);

    uint32_t written, ec;
    {
        std::tie(written, ec) = co_await write_awaitable(self_ptr,
                                                      *io_engine,
                                                      std::move(resume_executor),
                                                      buffer,
                                                      length,
                                                      self.m_write_pos,
                                                      optional_stop_token);
    }

    if (ec != 0) {
        if (ec == ERROR_OPERATION_ABORTED) {
            throw io_cancelled(details::consts::k_file_stream_write_io_cancelled_err_msg);
        }

        throw_system_error(ec);
    }

    if (written != 0 && self.m_write_pos != static_cast<size_t>(-1)) {
        self.m_write_pos += written;
    }

    co_return written;
}

lazy_result<void> file_stream_state::seek_read_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                   std::shared_ptr<concurrencpp::executor> resume_executor,
                                                   std::intptr_t pos,
                                                   seek_direction mode) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;
    auto sg = co_await self.m_lock.lock(resume_executor);

    ::LARGE_INTEGER size = {};
    const auto res = ::GetFileSizeEx(self.handle(), &size);
    if (res == FALSE) {
        throw_system_error(::GetLastError());
    }

    const auto current_pos = self.m_read_pos;
    const auto abs_pos = static_cast<size_t>(std::abs(pos));
    const auto file_size = static_cast<size_t>(size.QuadPart);

    if (mode == seek_direction::from_current) {
        if (pos < 0) {
            if (abs_pos > current_pos) {
                throw std::invalid_argument(details::consts::k_file_stream_seek_read_pos_new_pos_negative_err_msg);
            }

            self.m_read_pos -= abs_pos;
            co_return;
        }

        if (pos == 0) {
            co_return;
        }

        if (current_pos + abs_pos > file_size) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_read_pos_new_pos_transcend_err_msg);
        }

        self.m_read_pos += abs_pos;
        co_return;
    }

    if (mode == seek_direction::from_start) {
        if (pos < 0) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_read_pos_new_pos_negative_err_msg);
        }

        if (abs_pos > file_size) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_read_pos_new_pos_transcend_err_msg);
        }

        self.m_read_pos = abs_pos;
        co_return;
    }

    if (mode == seek_direction::from_end) {
        if (pos > 0) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_read_pos_new_pos_transcend_err_msg);
        }

        if (pos == 0) {
            self.m_read_pos = file_size;
            co_return;
        }

        if (abs_pos > file_size) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_read_pos_new_pos_negative_err_msg);
        }

        self.m_read_pos = (file_size - abs_pos);
        co_return;
    }

    assert(false);
}

lazy_result<std::size_t> file_stream_state::get_read_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                         std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;
    auto sg = co_await self.m_lock.lock(resume_executor);
    co_return self.m_read_pos;
}

lazy_result<void> file_stream_state::seek_write_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                    std::shared_ptr<concurrencpp::executor> resume_executor,
                                                    std::intptr_t pos,
                                                    seek_direction mode) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;
    auto sg = co_await self.m_lock.lock(resume_executor);

    ::LARGE_INTEGER size = {};
    const auto res = ::GetFileSizeEx(self.handle(), &size);
    if (res == 0) {
        throw_system_error(::GetLastError());
    }

    const auto current_pos = self.m_write_pos;
    const auto abs_pos = static_cast<size_t>(std::abs(pos));
    const auto file_size = static_cast<size_t>(size.QuadPart);

    if (mode == seek_direction::from_current) {
        if (pos < 0) {
            if (abs_pos > current_pos) {
                throw std::invalid_argument(details::consts::k_file_stream_seek_write_pos_new_write_pos_negative_err_msg);
            }

            self.m_write_pos -= abs_pos;
            co_return;
        }

        if (pos == 0) {
            co_return;
        }

        // if (current_pos + abs_pos > file_size) {} - it's ok in the case of writing, the file will be extended automatically
        self.m_write_pos += abs_pos;
        co_return;
    }

    if (mode == seek_direction::from_start) {
        if (pos < 0) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_write_pos_new_write_pos_negative_err_msg);
        }

        // if (abs_pos > file_size) {} as above
        self.m_write_pos = abs_pos;
        co_return;
    }

    if (mode == seek_direction::from_end) {
        // if (pos > 0) {} as above

        if (pos == 0) {
            self.m_write_pos = file_size;
            co_return;
        }

        if (pos < 0 && abs_pos > file_size) {
            throw std::invalid_argument(details::consts::k_file_stream_seek_write_pos_new_write_pos_negative_err_msg);
        }

        self.m_write_pos = (file_size - abs_pos);
        co_return;
    }

    assert(false);
}

lazy_result<std::size_t> file_stream_state::get_write_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                          std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto sg = co_await self_ptr->m_lock.lock(resume_executor);
    co_return self_ptr->m_write_pos;
}

lazy_result<void> file_stream_state::flush(std::shared_ptr<file_stream_state> self_ptr,
                                           std::shared_ptr<concurrencpp::executor> resume_executor,
                                           std::shared_ptr<io_engine> io_engine) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;
    //  auto sg = co_await self.m_lock.lock(engine->get_background_executor());

    const auto res = ::FlushFileBuffers(self.handle());

    co_await resume_on(resume_executor);

    if (res == FALSE) {
        throw_system_error(::GetLastError());
    }
}
