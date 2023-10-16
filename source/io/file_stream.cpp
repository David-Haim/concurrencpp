#include "concurrencpp/io/constants.h"
#include "concurrencpp/io/file_stream.h"
#include "concurrencpp/io/impl/win32/file_stream_state.h"

using concurrencpp::io_engine;
using concurrencpp::lazy_result;
using concurrencpp::file_stream;
using concurrencpp::file_open_mode;
using concurrencpp::details::file_stream_state;

file_stream::file_stream(std::shared_ptr<io_engine> io_engine, std::filesystem::path file_path, file_open_mode open_mode) {

    if (!static_cast<bool>(io_engine)) {
        throw std::invalid_argument(details::consts::k_file_stream_constructor_null_io_engine_msg);
    }

    if (file_path.empty()) {
        throw std::invalid_argument(details::consts::k_file_stream_constructor_empty_file_path_msg);
    }

    m_state = std::make_shared<file_stream_state>(std::move(file_path), open_mode, std::move(io_engine));
}

void file_stream::throw_if_empty(const char* error_message) const {
    if (!static_cast<bool>(m_state)) {
        throw std::runtime_error(error_message);
    }
}

void file_stream::throw_if_resume_executor_null(const std::shared_ptr<executor>& exec, const char* error_message) {
    if (!static_cast<bool>(exec)) {
        throw std::invalid_argument(error_message);
    }
}

void file_stream::swap(file_stream&& rhs) noexcept {
    std::swap(m_state, rhs.m_state);
}

void file_stream::close() noexcept {
    m_state.reset();
}

bool file_stream::empty() const noexcept {
    return static_cast<bool>(m_state);
}

lazy_result<bool> file_stream::eof_reached(std::shared_ptr<concurrencpp::executor> resume_executor) const {
    throw_if_empty(details::consts::k_file_stream_empty_get_eof_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_get_eof_null_executor_err_msg);

    return file_stream_state::eof_reached(m_state, std::move(resume_executor));
}

std::filesystem::path file_stream::get_path() const {
    throw_if_empty(details::consts::k_file_stream_empty_get_path_err_msg);

    return m_state->get_path();
}

file_open_mode file_stream::get_mode() const {
    throw_if_empty(details::consts::k_file_stream_empty_get_mode_err_msg);

    return m_state->get_open_mode();
}

lazy_result<void> file_stream::seek_read_pos(std::shared_ptr<concurrencpp::executor> resume_executor,
                                             std::intptr_t pos,
                                             seek_direction mode) {
    throw_if_empty(details::consts::k_file_stream_empty_seek_read_pos_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_seek_read_pos_null_resume_executor_err_msg);

    return file_stream_state::seek_read_pos(m_state, std::move(resume_executor), pos, mode);
}

lazy_result<std::size_t> file_stream::get_read_pos(std::shared_ptr<concurrencpp::executor> resume_executor) const {
    throw_if_empty(details::consts::k_file_stream_empty_get_read_pos_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_get_read_pos_null_resume_executor_err_msg);

    return file_stream_state::get_read_pos(m_state, std::move(resume_executor));
}

lazy_result<void> file_stream::seek_write_pos(std::shared_ptr<concurrencpp::executor> resume_executor,
                                              std::intptr_t pos,
                                              seek_direction mode) {
    throw_if_empty(details::consts::k_file_stream_empty_seek_write_pos_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_seek_write_pos_null_resume_executor_err_msg);

    return file_stream_state::seek_write_pos(m_state, std::move(resume_executor), pos, mode);
}

lazy_result<std::size_t> file_stream::get_write_pos(std::shared_ptr<concurrencpp::executor> resume_executor) const {
    throw_if_empty(details::consts::k_file_stream_empty_get_write_pos_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_get_write_pos_null_resume_executor_err_msg);

    return file_stream_state::get_write_pos(m_state, std::move(resume_executor));
}

lazy_result<size_t> file_stream::read_impl(std::shared_ptr<executor> resume_executor, void* buffer, size_t count) {
    throw_if_empty(details::consts::k_file_stream_empty_read_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_read_null_resume_executor_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument(details::consts::k_file_stream_read_invalid_buffer_err_msg);
    }

    auto engine = m_state->get_engine(details::consts::k_file_stream_read_io_engine_shutdown_err_msg);

    return file_stream_state::read(m_state, std::move(resume_executor), std::move(engine), buffer, count);
}

lazy_result<size_t> file_stream::read_impl(std::shared_ptr<executor> resume_executor,
                                           std::stop_token stop_token,
                                           void* buffer,
                                           size_t count) {
    throw_if_empty(details::consts::k_file_stream_empty_read_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_read_null_resume_executor_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument(details::consts::k_file_stream_read_invalid_buffer_err_msg);
    }

    if (!stop_token.stop_possible()) {
        throw std::invalid_argument(details::consts::k_file_stream_read_invalid_stop_token_err_msg);
    }

    auto engine = m_state->get_engine(details::consts::k_file_stream_read_io_engine_shutdown_err_msg);

    return file_stream_state::read(m_state, std::move(resume_executor), std::move(engine), buffer, count, &stop_token);
}

lazy_result<size_t> file_stream::write_impl(std::shared_ptr<executor> resume_executor, const void* buffer, size_t count) {
    throw_if_empty(details::consts::k_file_stream_empty_write_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_write_null_resume_executor_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument(details::consts::k_file_stream_write_invalid_buffer_err_msg);
    }

    auto engine = m_state->get_engine(details::consts::k_file_stream_write_engine_shutdown_err_msg);

    return file_stream_state::write(m_state, std::move(resume_executor), std::move(engine), buffer, count);
}

lazy_result<size_t> file_stream::write_impl(std::shared_ptr<executor> resume_executor,
                                            std::stop_token stop_token,
                                            const void* buffer,
                                            size_t count) {
    throw_if_empty(details::consts::k_file_stream_empty_write_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_write_null_resume_executor_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument(details::consts::k_file_stream_write_invalid_buffer_err_msg);
    }

    if (!stop_token.stop_possible()) {
        throw std::invalid_argument(details::consts::k_file_stream_write_invalid_stop_token_err_msg);
    }

    auto engine = m_state->get_engine(details::consts::k_file_stream_write_engine_shutdown_err_msg);

    return file_stream_state::write(m_state, std::move(resume_executor), std::move(engine), buffer, count, &stop_token);
}

lazy_result<void> file_stream::flush(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty(details::consts::k_file_stream_empty_flush_err_msg);
    throw_if_resume_executor_null(resume_executor, details::consts::k_file_stream_flush_null_resume_executor_err_msg);

    auto engine = m_state->get_engine(details::consts::k_file_stream_flush_engine_shutdown_err_msg);

    return file_stream_state::flush(m_state, std::move(resume_executor), std::move(engine));
}
