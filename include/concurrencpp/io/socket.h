#ifndef CONCURRENCPP_IO_SOCKET_H
#define CONCURRENCPP_IO_SOCKET_H

#include "concurrencpp/io/stream.h"
#include "concurrencpp/io/impl/win32/socket_state.h"
#include "concurrencpp/io/impl/common/socket_constants.h"

namespace concurrencpp {
    class socket : public io::stream {

       private:
        std::shared_ptr<details::win32::socket_state> m_state;

        void throw_if_empty(const char* error_msg) const;
        static void throw_if_resume_executor_empty(const std::shared_ptr<executor>& executor, const char* error_msg);
        static void throw_if_stop_token_empty(const std::stop_token& stop_token, const char* error_msg);

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

        lazy_result<void> connect(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint remote_endpoint);
        lazy_result<void> connect(std::shared_ptr<concurrencpp::executor> resume_executor,
                                  concurrencpp::ip_endpoint remote_endpoint,
                                  std::stop_token stop_token);

        lazy_result<void> listen(std::shared_ptr<concurrencpp::executor> resume_executor,
                                 std::uint32_t backlog = std::numeric_limits<uint32_t>::max());

        lazy_result<socket> accept(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<socket> accept(std::shared_ptr<concurrencpp::executor> resume_executor, std::stop_token stop_token);

        lazy_result<void> bind(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint local_endpoint);

        lazy_result<uint32_t> send_to(std::shared_ptr<executor> resume_executor, ip_endpoint ep, void* buffer, size_t count);
        lazy_result<uint32_t> send_to(std::shared_ptr<executor> resume_executor,
                                      std::stop_token stop_token,
                                      ip_endpoint ep,
                                      void* buffer,
                                      size_t count);

        lazy_result<concurrencpp::recv_from_result> receive_from(std::shared_ptr<executor> resume_executor,
                                                                 void* buffer,
                                                                 size_t count);
        lazy_result<concurrencpp::recv_from_result> receive_from(std::shared_ptr<executor> resume_executor,
                                                                 std::stop_token stop_token,
                                                                 void* buffer,
                                                                 size_t count);

        lazy_result<std::optional<ip_endpoint>> local_endpoint(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<std::optional<ip_endpoint>> remote_endpoint(std::shared_ptr<concurrencpp::executor> resume_executor);

        lazy_result<uint32_t> available_bytes(std::shared_ptr<concurrencpp::executor> resume_executor);

        lazy_result<bool> keep_alive(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> keep_alive(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);

        lazy_result<bool> broadcast_enabled(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> broadcast_enabled(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);

        lazy_result<bool> reuse_port(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> reuse_port(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);

        lazy_result<bool> linger_mode(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> linger_mode(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);

        lazy_result<bool> no_delay(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> no_delay(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);

        lazy_result<uint32_t> receive_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> receive_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor, uint32_t size);

        lazy_result<uint32_t> send_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> send_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor, uint32_t size);

        lazy_result<std::chrono::milliseconds> receive_timeout(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> receive_timeout(std::shared_ptr<concurrencpp::executor> resume_executor, std::chrono::milliseconds ms);

        lazy_result<std::chrono::milliseconds> send_timeout(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> send_timeout(std::shared_ptr<concurrencpp::executor> resume_executor, std::chrono::milliseconds ms);

        lazy_result<bool> dont_fragment(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> dont_fragment(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);

        lazy_result<uint16_t> ttl(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> ttl(std::shared_ptr<concurrencpp::executor> resume_executor, uint16_t ttl);

        lazy_result<bool> multicast_loopback(std::shared_ptr<concurrencpp::executor> resume_executor);
        lazy_result<void> multicast_loopback(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable);
    };
}  // namespace concurrencpp

#endif
