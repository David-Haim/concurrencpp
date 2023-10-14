#ifndef CONCURRENCPP_WIN32_SOCKET_STATE_H
#define CONCURRENCPP_WIN32_SOCKET_STATE_H

#include "concurrencpp/io/ip.h"
#include "concurrencpp/io/forward_declarations.h"
#include "concurrencpp/io/impl/win32/io_object.h"
#include "concurrencpp/io/impl/common/socket_constants.h"

#include <optional>
#include <stop_token>

namespace concurrencpp {
    struct recv_from_result {
        ip_endpoint remote_endpoint;
        uint32_t read = 0;
    };
}  // namespace concurrencpp

// TODO: move all error msgs to constants file
namespace concurrencpp::details::win32 {

    using namespace concurrencpp::io;

    class socket_state final : public io_object {

       private:
        const address_family m_address_family;
        const socket_type m_socket_type;
        const protocol_type m_protocol_type;
        std::optional<ip_endpoint> m_local_endpoint;
        std::optional<ip_endpoint> m_remote_endpoint;
        bool m_eof_reached = false;

        std::optional<ip_endpoint> query_local_address() const;

        void get_socket_op(int level, int opt, void* dst, int* len) const;
        void set_socket_op(int level, int opt, void* src, int len);

        static void* open_socket(address_family af, socket_type type, protocol_type protocol);
        static void close_socket(void*) noexcept;

        std::uintptr_t fd() const noexcept;

       public:
        socket_state(std::shared_ptr<io_engine> engine, address_family address_family, socket_type type, protocol_type protocol);

        address_family get_address_family() const noexcept;
        socket_type get_socket_type() const noexcept;
        protocol_type get_protocol_type() const noexcept;

        static lazy_result<bool> eof_reached(std::shared_ptr<socket_state> self,
                                             std::shared_ptr<concurrencpp::executor> resume_executor);

        static lazy_result<size_t> read(std::shared_ptr<socket_state> self,
                                        std::shared_ptr<concurrencpp::executor> resume_executor,
                                        std::shared_ptr<io_engine> io_engine,
                                        void* buffer,
                                        uint32_t buffer_length,
                                        std::stop_token* optional_stop_token = nullptr);

        static lazy_result<size_t> write(std::shared_ptr<socket_state> self,
                                         std::shared_ptr<concurrencpp::executor> resume_executor,
                                         std::shared_ptr<io_engine> io_engine,
                                         const void* buffer,
                                         uint32_t buffer_length,
                                         std::stop_token* optional_stop_token = nullptr);

        static lazy_result<void> connect(std::shared_ptr<socket_state> self,
                                         std::shared_ptr<concurrencpp::executor> resume_executor,
                                         std::shared_ptr<io_engine> io_engine,
                                         ip_endpoint remote_endpoint,
                                         std::stop_token* optional_stop_token = nullptr);

        static lazy_result<concurrencpp::socket> accept(std::shared_ptr<socket_state> self,
                                                        std::shared_ptr<concurrencpp::executor> resume_executor,
                                                        std::shared_ptr<io_engine> io_engine,
                                                        std::stop_token* optional_stop_token = nullptr);
        // TODO: cancellation
        static lazy_result<void> bind(std::shared_ptr<socket_state> self,
                                      std::shared_ptr<concurrencpp::executor> resume_executor,
                                      ip_endpoint local_endpoint);

        // TODO: cancellation
        static lazy_result<void> listen(std::shared_ptr<socket_state> self,
                                        std::shared_ptr<concurrencpp::executor> resume_executor,
                                        std::uint32_t backlog);

        static lazy_result<uint32_t> send_to(std::shared_ptr<socket_state> self,
                                             std::shared_ptr<concurrencpp::executor> resume_executor,
                                             std::shared_ptr<io_engine> io_engine,
                                             void* buffer,
                                             uint32_t buffer_length,
                                             ip_endpoint endpoint,
                                             std::stop_token* optional_stop_token = nullptr);

        static lazy_result<recv_from_result> recv_from(std::shared_ptr<socket_state> self,
                                                       std::shared_ptr<concurrencpp::executor> resume_executor,
                                                       std::shared_ptr<io_engine> io_engine,
                                                       void* buffer,
                                                       uint32_t buffer_length,
                                                       std::stop_token* optional_stop_token = nullptr);

        static lazy_result<std::optional<ip_endpoint>> local_endpoint(std::shared_ptr<socket_state> self,
                                                                      std::shared_ptr<concurrencpp::executor> resume_executor);

        static lazy_result<std::optional<ip_endpoint>> remote_endpoint(std::shared_ptr<socket_state> self,
                                                                       std::shared_ptr<concurrencpp::executor> resume_executor);

        static lazy_result<uint32_t> available_bytes(std::shared_ptr<socket_state> self,
                                                     std::shared_ptr<concurrencpp::executor> resume_executor);

        static lazy_result<bool> keep_alive(std::shared_ptr<socket_state> self,
                                            std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> keep_alive(std::shared_ptr<socket_state> self,
                                            std::shared_ptr<concurrencpp::executor> resume_executor,
                                            bool enable);

        static lazy_result<bool> broadcast_enabled(std::shared_ptr<socket_state> self,
                                                   std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> broadcast_enabled(std::shared_ptr<socket_state> self,
                                                   std::shared_ptr<concurrencpp::executor> resume_executor,
                                                   bool enable);

        static lazy_result<bool> reuse_port(std::shared_ptr<socket_state> self,
                                            std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> reuse_port(std::shared_ptr<socket_state> self,
                                            std::shared_ptr<concurrencpp::executor> resume_executor,
                                            bool enable);

        static lazy_result<bool> linger_mode(std::shared_ptr<socket_state> self,
                                             std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> linger_mode(std::shared_ptr<socket_state> self,
                                             std::shared_ptr<concurrencpp::executor> resume_executor,
                                             bool enable);

        static lazy_result<bool> no_delay(std::shared_ptr<socket_state> self, std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> no_delay(std::shared_ptr<socket_state> self,
                                          std::shared_ptr<concurrencpp::executor> resume_executor,
                                          bool enable);

        static lazy_result<uint32_t> receive_buffer_size(std::shared_ptr<socket_state> self,
                                                         std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> receive_buffer_size(std::shared_ptr<socket_state> self,
                                                     std::shared_ptr<concurrencpp::executor> resume_executor,
                                                     uint32_t size);

        static lazy_result<uint32_t> send_buffer_size(std::shared_ptr<socket_state> self,
                                                      std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> send_buffer_size(std::shared_ptr<socket_state> self,
                                                  std::shared_ptr<concurrencpp::executor> resume_executor,
                                                  uint32_t size);

        static lazy_result<std::chrono::milliseconds> receive_timeout(std::shared_ptr<socket_state> self,
                                                                      std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> receive_timeout(std::shared_ptr<socket_state> self,
                                                 std::shared_ptr<concurrencpp::executor> resume_executor,
                                                 std::chrono::milliseconds ms);

        static lazy_result<std::chrono::milliseconds> send_timeout(std::shared_ptr<socket_state> self,
                                                                   std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> send_timeout(std::shared_ptr<socket_state> self,
                                              std::shared_ptr<concurrencpp::executor> resume_executor,
                                              std::chrono::milliseconds ms);

        static lazy_result<bool> dont_fragment(std::shared_ptr<socket_state> self,
                                               std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> dont_fragment(std::shared_ptr<socket_state> self,
                                               std::shared_ptr<concurrencpp::executor> resume_executor,
                                               bool enable);

        static lazy_result<uint16_t> ttl(std::shared_ptr<socket_state> self, std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> ttl(std::shared_ptr<socket_state> self,
                                     std::shared_ptr<concurrencpp::executor> resume_executor,
                                     uint16_t ttl);

        static lazy_result<bool> multicast_loopback(std::shared_ptr<socket_state> self,
                                                    std::shared_ptr<concurrencpp::executor> resume_executor);
        static lazy_result<void> multicast_loopback(std::shared_ptr<socket_state> self,
                                                    std::shared_ptr<concurrencpp::executor> resume_executor,
                                                    bool enable);
    };
}  // namespace concurrencpp::details::win32

#endif