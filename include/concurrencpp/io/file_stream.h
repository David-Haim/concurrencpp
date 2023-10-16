#ifndef CONCURRENCPP_FILE_STREAM_H
#define CONCURRENCPP_FILE_STREAM_H

#include "concurrencpp/io/stream.h"
#include "concurrencpp/executors/executor.h"
#include "concurrencpp/io/forward_declarations.h"
#include "concurrencpp/io/impl/common/file_constants.h"

#include <filesystem>

// TODO fix flush
namespace concurrencpp {
    class file_stream final : public io::stream {

       private:
        std::shared_ptr<details::win32::file_stream_state> m_state;

        void throw_if_empty(const char* error_message) const;
        static void throw_if_resume_executor_null(const std::shared_ptr<executor>& exec, const char* error_message);

       protected:
        lazy_result<size_t> read_impl(std::shared_ptr<executor> resume_executor, void* buffer, size_t count);
        lazy_result<size_t> read_impl(std::shared_ptr<executor> resume_executor,
                                      std::stop_token stop_token,
                                      void* buffer,
                                      size_t count);

        lazy_result<size_t> write_impl(std::shared_ptr<executor> resume_executor, const void* buffer, size_t count);
        lazy_result<size_t> write_impl(std::shared_ptr<executor> resume_executor,
                                       std::stop_token stop_token,
                                       const void* buffer,
                                       size_t count);

       public:
        file_stream(std::shared_ptr<io_engine> io_engine, std::filesystem::path file_path, file_open_mode open_mode);
        ~file_stream() noexcept = default;

        file_stream(file_stream&& rhs) noexcept = default;
        file_stream& operator=(file_stream&& rhs) noexcept = default;

        file_stream(const file_stream&) = delete;
        file_stream& operator=(const file_stream& rhs) = delete;

        void swap(file_stream&& rhs) noexcept;

        void close() noexcept;

        bool empty() const noexcept;
        std::filesystem::path get_path() const;
        file_open_mode get_mode() const;

        lazy_result<bool> eof_reached(std::shared_ptr<executor> resume_executor) const;

        lazy_result<void> seek_read_pos(std::shared_ptr<executor> resume_executor,
                                        std::intptr_t pos,
                                        seek_direction mode = seek_direction::from_current);

        lazy_result<std::size_t> get_read_pos(std::shared_ptr<executor> resume_executor) const;

        lazy_result<void> seek_write_pos(std::shared_ptr<executor> resume_executor,
                                         std::intptr_t pos,
                                         seek_direction mode = seek_direction::from_current);
        lazy_result<std::size_t> get_write_pos(std::shared_ptr<executor> resume_executor) const;

        lazy_result<void> flush(std::shared_ptr<executor> resume_executor);
    };
}  // namespace concurrencpp

#endif