#ifndef CONCURRENCPP_IO_SOCKET_H
#define CONCURRENCPP_IO_SOCKET_H

#include "concurrencpp/io/impl/win32/socket_state.h"
#include "concurrencpp/io/impl/common/reader_writer.h"
#include "concurrencpp/io/impl/common/socket_constants.h"

namespace concurrencpp {
    class socket : public io::details::reader_writer<socket, details::win32::socket_state> {

       private:
        friend class io::details::reader_writer<socket, details::win32::socket_state>;

        void throw_if_empty(const char* error_msg) const;

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
        socket() noexcept = default;
        socket(std::shared_ptr<details::win32::socket_state> state) noexcept;

        socket(std::shared_ptr<io_engine> engine, io::address_family af, io::socket_type type, io::protocol_type protocol);

        socket(socket&& rhs) noexcept = default;
        socket& operator=(socket&& rhs) noexcept = default;

        void close() noexcept;
        bool empty() const noexcept;

        io::address_family get_address_family() const;
        io::socket_type get_socket_type() const;
        io::protocol_type get_protocol_type() const;

        lazy_result<bool> eof_reached(std::shared_ptr<concurrencpp::executor> resume_executor);

        lazy_result<void> connect(std::shared_ptr<concurrencpp::executor> resume_executor,
                                                concurrencpp::ip_endpoint remote_endpoint);
        lazy_result<void> connect(std::shared_ptr<concurrencpp::executor> resume_executor,
                                                concurrencpp::ip_endpoint remote_endpoint,
                                                std::stop_token stop_token);

        lazy_result<void> listen(std::shared_ptr<concurrencpp::executor> resume_executor,
                                               std::uint32_t backlog = std::numeric_limits<uint32_t>::max());

        lazy_result<socket> accept(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<socket> accept(std::shared_ptr<concurrencpp::executor> resume_executor, std::stop_token stop_token);

        lazy_result<void> bind(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint local_endpoint);

        bool keep_alive() const;
        void keep_alive(bool enable);

        bool broadcast_enabled() const;
        void broadcast_enabled(bool enable);

        bool reuse_port() const;
        void reuse_port(bool enable);

        bool linger_mode() const;
        void linger_mode(bool enable);

        bool no_delay() const;
        void no_delay(bool enable);

        uint32_t receive_buffer_size() const;
        void receive_buffer_size(uint32_t size);

        uint32_t send_buffer_size() const;
        void send_buffer_size(uint32_t size);

        std::chrono::milliseconds receive_timeout() const;
        void receive_timeout(std::chrono::milliseconds ms);

        std::chrono::milliseconds send_timeout() const;
        void send_timeout(std::chrono::milliseconds ms);
    };
}  // namespace concurrencpp

#endif
