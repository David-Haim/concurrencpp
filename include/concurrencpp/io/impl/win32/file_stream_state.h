#ifndef CONCURRENCPP_WIN32_FILE_STREAM_STATE_H
#define CONCURRENCPP_WIN32_FILE_STREAM_STATE_H

#include "concurrencpp/io/impl/win32/io_object.h"
#include "concurrencpp/io/impl/common/file_constants.h"

#include <filesystem>
#include <stop_token>

namespace concurrencpp::details::win32 {
    class file_stream_state final : public io_object {

       private:
        const std::filesystem::path m_path;
        const file_open_mode m_open_mode;
        std::size_t m_read_pos = 0;
        std::size_t m_write_pos = 0;
        bool m_eof_reached = false;

        static void* open_file_handle(const std::filesystem::path& path, file_open_mode mode);
        static void close_file_handle(void* handle) noexcept;

       public:
        file_stream_state(std::filesystem::path path, file_open_mode open_mode, std::shared_ptr<io_engine> engine) noexcept;

        std::filesystem::path get_path() const noexcept;
        file_open_mode get_open_mode() const noexcept;

        // TODO cancellable ops
        static lazy_result<bool> eof_reached(std::shared_ptr<file_stream_state> self, std::shared_ptr<executor> resume_executor);

        static lazy_result<size_t> read(std::shared_ptr<file_stream_state> self_ptr,
                                        std::shared_ptr<executor> resume_executor,
                                        std::shared_ptr<io_engine> io_engine,
                                        void* buffer,
                                        size_t buffer_length,
                                        std::stop_token* optional_stop_token = nullptr);

        static lazy_result<size_t> write(std::shared_ptr<file_stream_state> self_ptr,
                                         std::shared_ptr<executor> resume_executor,
                                         std::shared_ptr<io_engine> io_engine,
                                         const void* buffer,
                                         size_t buffer_length,
                                         std::stop_token* optional_stop_token = nullptr);

        static lazy_result<void> flush(std::shared_ptr<file_stream_state> self_ptr,
                                       std::shared_ptr<concurrencpp::executor> resume_executor,
                                       std::shared_ptr<io_engine> io_engine);

        static lazy_result<void> seek_read_pos(std::shared_ptr<file_stream_state> self_ptr,
                                               std::shared_ptr<executor> resume_executor,
                                               std::intptr_t pos,
                                               seek_direction mode);

        static lazy_result<std::size_t> get_read_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                     std::shared_ptr<executor> resume_executor);

        static lazy_result<void> seek_write_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                std::shared_ptr<concurrencpp::executor> resume_executor,
                                                std::intptr_t pos,
                                                seek_direction mode = seek_direction::from_current);

        static lazy_result<std::size_t> get_write_pos(std::shared_ptr<file_stream_state> self_ptr,
                                                      std::shared_ptr<executor> resume_executor);
    };
}  // namespace concurrencpp::details::win32

#endif