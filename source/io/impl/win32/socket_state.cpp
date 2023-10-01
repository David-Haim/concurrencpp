#include "concurrencpp/io/socket.h"

#include "concurrencpp/errors.h"
#include "concurrencpp/io/impl/win32/utils.h"
#include "concurrencpp/io/impl/win32/io_awaitable.h"
#include "concurrencpp/io/impl/win32/socket_state.h"

#include <cassert>

#define NOMINMAX
#include <WinSock2.h>
#include <ws2ipdef.h>

using concurrencpp::lazy_result;
using concurrencpp::details::win32::socket_state;

socket_state::socket_state(std::shared_ptr<io_engine> engine,
                           address_family address_family,
                           socket_type type,
                           protocol_type protocol) :
    io_object(open_socket(address_family, type, protocol), close_socket, std::move(engine)),
    m_address_family(address_family), m_socket_type(type), m_protocol_type(protocol) {
    assert(fd() != INVALID_SOCKET);
}

void* socket_state::open_socket(address_family af, socket_type type, protocol_type protocol) {
    int sock_af = 0;
    if (af == address_family::ip_v4) {
        sock_af = AF_INET;
    } else if (af == address_family::ip_v6) {
        sock_af = AF_INET6;
    } else {
        assert(false);
    }

    int sock_type = 0;
    switch (type) {
        case socket_type::raw: {
            sock_type = SOCK_RAW;
            break;
        }
        case socket_type::datagram: {
            sock_type = SOCK_DGRAM;
            break;
        }
        case socket_type::stream: {
            sock_type = SOCK_STREAM;
            break;
        }
        default: {
            assert(false);
        }
    }

    int sock_prot = 0;
    switch (protocol) {
        case protocol_type::tcp: {
            sock_prot = IPPROTO_TCP;
            break;
        }
        case protocol_type::udp: {
            sock_prot = IPPROTO_UDP;
            break;
        }
        case protocol_type::icmp: {
            sock_prot = IPPROTO_ICMP;
            break;
        }
        case protocol_type::icmp_v6: {
            sock_prot = IPPROTO_ICMPV6;
            break;
        }
        default: {
            assert(false);
        }
    }

    const auto socket_handle = ::WSASocketW(sock_af, sock_type, sock_prot, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket_handle == INVALID_SOCKET) {
        throw_system_error(::WSAGetLastError());
    }

    return reinterpret_cast<void*>(socket_handle);
}

void socket_state::close_socket(void* handle) noexcept {
    ::CancelIoEx(handle, nullptr);
    const auto res = ::closesocket(reinterpret_cast<SOCKET>(handle));
    assert(res == 0);
}

concurrencpp::io::address_family socket_state::get_address_family() const noexcept {
    return m_address_family;
}

concurrencpp::io::socket_type socket_state::get_socket_type() const noexcept {
    return m_socket_type;
}

concurrencpp::io::protocol_type socket_state::get_protocol_type() const noexcept {
    return m_protocol_type;
}

std::uintptr_t socket_state::fd() const noexcept {
    return reinterpret_cast<std::uintptr_t>(handle());
}

lazy_result<bool> socket_state::eof_reached(
    std::shared_ptr<socket_state> self,
    std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self));
    assert(static_cast<bool>(resume_executor));
    
    // TODO: cancellation
    auto sg = co_await self->m_lock.lock(resume_executor);
    co_return self->m_eof_reached;
}

std::optional<concurrencpp::ip_endpoint> socket_state::query_local_address() const {
    ::sockaddr_storage local_address = {};
    auto local_address_len = static_cast<int>(sizeof (local_address));

    const auto res = ::getsockname(fd(), reinterpret_cast<::sockaddr*>(&local_address), &local_address_len);
    if (res != 0) {
        const auto error_code = ::GetLastError();
        if (error_code == WSAEINVAL) {
            return std::nullopt;
        }

        assert(error_code != WSANOTINITIALISED);
        assert(error_code != WSAEFAULT);
        assert(error_code != WSAENOTSOCK);
        throw_system_error(error_code);
    }

    if (local_address.ss_family == AF_INET) {
        auto& ipv4_address = *reinterpret_cast<sockaddr_in*>(&local_address);
        const std::uint32_t bound_ip = ipv4_address.sin_addr.S_un.S_addr;
        const std::uint16_t bound_port = ::ntohs(ipv4_address.sin_port);
        return ip_endpoint {bound_ip, bound_port};
    }

    assert(local_address.ss_family == AF_INET6);
    auto& ipv6_address = *reinterpret_cast<sockaddr_in6*>(&local_address);

    const auto address_bytes =
        std::span<const std::byte, 16> {reinterpret_cast<const std::byte*>(&ipv6_address.sin6_addr.u.Byte[0]), 16};
    const std::uint16_t bound_port = ::ntohs(ipv6_address.sin6_port);
    const std::uint32_t scope_id = ipv6_address.sin6_scope_id;
    return ip_endpoint(ip_v6 {address_bytes, scope_id}, bound_port);
}

void socket_state::get_socket_op(int level, int opt, void* dst, int* len) const {
    const auto res = ::getsockopt(fd(), level, opt, static_cast<char*>(dst), len);
    if (res != 0) {
        throw_system_error(::WSAGetLastError());
    }
}

void socket_state::set_socket_op(int level, int opt, void* src, int len) {
    const auto res = ::setsockopt(fd(), level, opt, static_cast<char*>(src), len);
    if (res != 0) {
        throw_system_error(::WSAGetLastError());
    }
}

lazy_result<size_t> socket_state::read(std::shared_ptr<socket_state> self_ptr,
                                       std::shared_ptr<concurrencpp::executor> resume_executor,
                                       std::shared_ptr<io_engine> io_engine,
                                       void* buffer,
                                       uint32_t buffer_length,
                                       std::stop_token* optional_stop_token) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

    // TODO: cancellation
    auto sg = co_await self.m_lock.lock(resume_executor);

    const size_t read_pos = 0;
    uint32_t read, ec;

    {  // it's important for the awaitable to be destroyed asap when io finishes for stored-stop_cb in it.
        std::tie(read, ec) =
            co_await read_awaitable(*self_ptr, *io_engine, *resume_executor, buffer, buffer_length, read_pos, optional_stop_token);
    }

    if (ec != 0) {
        throw_system_error(ec);
    }

    if (read == 0) {
        self.m_eof_reached = true;
    }

    co_return read;
}

lazy_result<size_t> socket_state::write(std::shared_ptr<socket_state> self_ptr,
                                        std::shared_ptr<concurrencpp::executor> resume_executor,
                                        std::shared_ptr<io_engine> io_engine,
                                        const void* buffer,
                                        uint32_t buffer_length,
                                        std::stop_token* optional_stop_token) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    const size_t write_pos = 0;
    uint32_t written, ec;

    {
        std::tie(written, ec) =
            co_await write_awaitable(*self_ptr, *io_engine, *resume_executor, buffer, buffer_length, write_pos, optional_stop_token);
    }

    if (ec != 0) {
        throw_system_error(ec);
    }

    co_return ec;
}

lazy_result<void> socket_state::connect(std::shared_ptr<socket_state> self_ptr,
                                        std::shared_ptr<concurrencpp::executor> resume_executor,
                                        std::shared_ptr<io_engine> io_engine,
                                        ip_endpoint remote_endpoint,
                                        std::stop_token* optional_stop_token) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    uint32_t ec;

    { ec = co_await connect_awaitable(*self_ptr, *io_engine, *resume_executor, remote_endpoint, optional_stop_token); }

    if (ec != 0) {
        throw_system_error(ec);
    }

    self.m_remote_endpoint.emplace(remote_endpoint);
    self.m_local_endpoint = self.query_local_address();
    assert(self.m_local_endpoint.has_value());
}

lazy_result<concurrencpp::socket> socket_state::accept(std::shared_ptr<socket_state> self_ptr,
                                                                std::shared_ptr<concurrencpp::executor> resume_executor,
                                                                std::shared_ptr<io_engine> io_engine,
                                                                std::stop_token* optional_stop_token) {

    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    // TODO: check that this is valid, the accepted socket will likely to have the acceptor properties
    auto accept_state = std::make_shared<socket_state>(io_engine, self.m_address_family, self.m_socket_type, self.m_protocol_type);

    ip_endpoint local_ep, remote_ep;
    uint32_t ec;

    {
        std::tie(local_ep, remote_ep, ec) =
            co_await accept_awaitable(*self_ptr, *io_engine, *resume_executor, accept_state->handle(), optional_stop_token);
    }

    if (ec != 0) {
        throw_system_error(ec);
    }

    accept_state->m_local_endpoint.emplace(local_ep);
    accept_state->m_remote_endpoint.emplace(remote_ep);
 
    co_return socket(std::move(accept_state));
}

lazy_result<void> socket_state::bind(std::shared_ptr<socket_state> self_ptr,
                                     std::shared_ptr<concurrencpp::executor> resume_executor,
                                     ip_endpoint local_endpoint) {

    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    ::sockaddr_storage storage = {};
    const auto addr_len = static_cast<int>(sizeof(storage));

    if (local_endpoint.address.index() == 0) {
        auto ipv4_address = new (&storage) ::sockaddr_in();
        ipv4_address->sin_family = AF_INET;
        ipv4_address->sin_addr.S_un.S_addr = std::get<0>(local_endpoint.address).address();
        ipv4_address->sin_port = ::htons(local_endpoint.port);
    } else {
        auto ipv6_address = new (&storage)::sockaddr_in6();
        ipv6_address->sin6_family = AF_INET6;
        ipv6_address->sin6_port = ::htons(local_endpoint.port);
        ipv6_address->sin6_scope_id = std::get<1>(local_endpoint.address).scope_id();
        const auto bytes = std::get<1>(local_endpoint.address).bytes();
        ::memcpy_s(&ipv6_address->sin6_addr.u.Byte[0], 16, bytes.data(), 16);
    }

    const auto res = ::bind(self.fd(), reinterpret_cast<::sockaddr*>(&storage), addr_len);
    if (res != 0) {
        throw_system_error(::WSAGetLastError());
    }

    self.m_local_endpoint.emplace(local_endpoint);
}

#undef max
lazy_result<void> socket_state::listen(std::shared_ptr<socket_state> self_ptr,
                                       std::shared_ptr<concurrencpp::executor> resume_executor,
                                       std::uint32_t backlog) {

    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));

    auto& self = *self_ptr;
    auto sg = co_await self.m_lock.lock(resume_executor);

    if (backlog == std::numeric_limits<uint32_t>::max()) {
        backlog = SOMAXCONN;
    }

    const auto res = ::listen(self.fd(), static_cast<int>(backlog));
    if (res != 0) {
        throw_system_error(::WSAGetLastError());
    }
}

lazy_result<uint32_t> socket_state::send_to(std::shared_ptr<socket_state> self_ptr,
                                            std::shared_ptr<concurrencpp::executor> resume_executor,
                                            std::shared_ptr<io_engine> io_engine,
                                            void* buffer,
                                            uint32_t buffer_length,
                                            ip_endpoint endpoint,
                                            std::stop_token* optional_stop_token) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

//    const auto engine = self.get_engine("concurrencpp::socket::send_to() - engine has been shut down.");

    auto sg = co_await self.m_lock.lock(resume_executor);

    uint32_t written, ec;

    {
        std::tie(written, ec) =
            co_await send_to_awaitable(*self_ptr, *io_engine, *resume_executor, buffer, buffer_length, endpoint, optional_stop_token);
    }

    if (ec != 0) {
        throw_system_error(ec);
    }

    // TODO: handle implicit binding here

    co_return written;
}

lazy_result<concurrencpp::recv_from_result> socket_state::recv_from(std::shared_ptr<socket_state> self_ptr,
                                                                    std::shared_ptr<concurrencpp::executor> resume_executor,
                                                                    std::shared_ptr<io_engine> io_engine,
                                                                    void* buffer,
                                                                    uint32_t buffer_length,
                                                                    std::stop_token* optional_stop_token) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    assert(static_cast<bool>(io_engine));

    auto& self = *self_ptr;

//    const auto engine = self.get_engine("concurrencpp::socket::recv_from() - engine has been shut down.");

    auto sg = co_await self.m_lock.lock(resume_executor);

    uint32_t read, ec;
    ::sockaddr_storage addr = {};

    {
        std::tie(read, ec, addr) =
            co_await recv_from_awaitable(*self_ptr, *io_engine, *resume_executor, buffer, buffer_length, optional_stop_token);
    }

    if (ec != 0) {
        throw_system_error(ec);
    }

    if (addr.ss_family == AF_INET) {
        const auto& addr_v4 = reinterpret_cast<sockaddr_in&>(addr);
        ip_v4 ip(addr_v4.sin_addr.S_un.S_addr);
        ip_endpoint ip_endpoint(ip, ::ntohl(addr_v4.sin_port));

        co_return recv_from_result {{ip_endpoint}, read};
    }

    assert(addr.ss_family == AF_INET6);
    const auto& addr_v6 = reinterpret_cast<sockaddr_in6&>(addr);

    std::span<const std::byte, 16> address_bytes(reinterpret_cast<const std::byte*>(addr_v6.sin6_addr.u.Byte), 16);
    ip_v6 ip(address_bytes, addr_v6.sin6_scope_id);
    ip_endpoint ip_endpoint(ip, ::ntohs(addr_v6.sin6_port));

    co_return recv_from_result {{ip_endpoint}, read};
}

lazy_result<std::optional<concurrencpp::ip_endpoint>> socket_state::local_endpoint(
    std::shared_ptr<socket_state> self_ptr,
    std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));

    auto sg = co_await self_ptr->m_lock.lock(resume_executor);
    co_return self_ptr->m_local_endpoint;
}

lazy_result<std::optional<concurrencpp::ip_endpoint>> socket_state::remote_endpoint(
    std::shared_ptr<socket_state> self_ptr,
    std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));

    auto sg = co_await self_ptr->m_lock.lock(resume_executor);
    co_return self_ptr->m_remote_endpoint;
}

lazy_result<uint32_t> socket_state::available_bytes(std::shared_ptr<socket_state> self_ptr,
                                                    std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    u_long bytes_available = 0;
    const auto res = ::ioctlsocket(self.fd(), FIONREAD, &bytes_available);
    if (res == 0) {
        co_return bytes_available;
    }

    throw_system_error(::WSAGetLastError());
}

lazy_result<bool> socket_state::keep_alive(std::shared_ptr<socket_state> self_ptr,
                                           std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;
    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = FALSE;
    auto len = static_cast<int>(sizeof enabled);
    self.get_socket_op(SOL_SOCKET, SO_KEEPALIVE, &enabled, &len);
    co_return enabled == TRUE;
}

lazy_result<void> socket_state::keep_alive(std::shared_ptr<socket_state> self_ptr,
                                           std::shared_ptr<concurrencpp::executor> resume_executor,
                                           bool enable) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = enable;
    const auto len = static_cast<int>(sizeof enabled);
    self.set_socket_op(SOL_SOCKET, SO_KEEPALIVE, &enabled, len);
}

lazy_result<bool> socket_state::broadcast_enabled(std::shared_ptr<socket_state> self_ptr,
                                                  std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = FALSE;
    auto len = static_cast<int>(sizeof enabled);
    self.get_socket_op(SOL_SOCKET, SO_BROADCAST, &enabled, &len);
    co_return enabled == TRUE;
}

lazy_result<void> socket_state::broadcast_enabled(std::shared_ptr<socket_state> self_ptr,
                                                  std::shared_ptr<concurrencpp::executor> resume_executor,
                                                  bool enable) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = enable;
    const auto len = static_cast<int>(sizeof enabled);
    self.set_socket_op(SOL_SOCKET, SO_BROADCAST, &enabled, len);
}

lazy_result<bool> socket_state::reuse_port(std::shared_ptr<socket_state> self_ptr,
                                           std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = FALSE;
    auto len = static_cast<int>(sizeof enabled);
    self.get_socket_op(SOL_SOCKET, SO_REUSEADDR, &enabled, &len);
    co_return enabled == TRUE;
}

lazy_result<void> socket_state::reuse_port(std::shared_ptr<socket_state> self_ptr,
                                           std::shared_ptr<concurrencpp::executor> resume_executor,
                                           bool enable) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = enable;
    const auto len = static_cast<int>(sizeof enabled);
    self.set_socket_op(SOL_SOCKET, SO_REUSEADDR, &enabled, len);
}

lazy_result<bool> socket_state::linger_mode(std::shared_ptr<socket_state> self_ptr,
                                            std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = FALSE;
    auto len = static_cast<int>(sizeof enabled);
    self.get_socket_op(SOL_SOCKET, SO_LINGER, &enabled, &len);
    co_return enabled == TRUE;
}

lazy_result<void> socket_state::linger_mode(std::shared_ptr<socket_state> self_ptr,
                                            std::shared_ptr<concurrencpp::executor> resume_executor,
                                            bool enable) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    // TODO: this is wrong, SO_LINGER != SO_DONTLINGER
    BOOL enabled = enable;
    const auto len = static_cast<int>(sizeof enabled);
    self.set_socket_op(SOL_SOCKET, SO_LINGER, &enabled, len);
}

lazy_result<bool> socket_state::no_delay(std::shared_ptr<socket_state> self_ptr,
                                         std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = FALSE;
    auto len = static_cast<int>(sizeof enabled);
    self.get_socket_op(IPPROTO_TCP, TCP_NODELAY, &enabled, &len);
    co_return enabled == TRUE;
}

lazy_result<void> socket_state::no_delay(std::shared_ptr<socket_state> self_ptr,
                                         std::shared_ptr<concurrencpp::executor> resume_executor,
                                         bool enable) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    BOOL enabled = enable;
    const auto len = static_cast<int>(sizeof enabled);
    self.set_socket_op(IPPROTO_TCP, TCP_NODELAY, &enabled, len);
}

lazy_result<uint32_t> socket_state::receive_buffer_size(std::shared_ptr<socket_state> self_ptr,
                                                        std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    int buffer_len = 0;
    auto len = static_cast<int>(sizeof buffer_len);
    self.get_socket_op(SOL_SOCKET, SO_RCVBUF, &buffer_len, &len);
    co_return buffer_len;
}

lazy_result<void> socket_state::receive_buffer_size(std::shared_ptr<socket_state> self_ptr,
                                                    std::shared_ptr<concurrencpp::executor> resume_executor,
                                                    uint32_t size) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    const auto len = static_cast<int>(sizeof size);
    self.set_socket_op(SOL_SOCKET, SO_RCVBUF, &size, len);
}

lazy_result<uint32_t> socket_state::send_buffer_size(std::shared_ptr<socket_state> self_ptr,
                                                     std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    int buffer_len = 0;
    auto len = static_cast<int>(sizeof buffer_len);
    self.get_socket_op(SOL_SOCKET, SO_SNDBUF, &buffer_len, &len);
    co_return buffer_len;
}

lazy_result<void> socket_state::send_buffer_size(std::shared_ptr<socket_state> self_ptr,
                                                 std::shared_ptr<concurrencpp::executor> resume_executor,
                                                 uint32_t size) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    const auto len = static_cast<int>(sizeof size);
    self.set_socket_op(SOL_SOCKET, SO_SNDBUF, &size, len);
}

lazy_result<std::chrono::milliseconds> socket_state::receive_timeout(std::shared_ptr<socket_state> self_ptr,
                                                                     std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    int timeout = 0;
    auto len = static_cast<int>(sizeof timeout);
    self.get_socket_op(SOL_SOCKET, SO_RCVTIMEO, &timeout, &len);
    co_return std::chrono::milliseconds(timeout);
}

lazy_result<void> socket_state::receive_timeout(std::shared_ptr<socket_state> self_ptr,
                                                std::shared_ptr<concurrencpp::executor> resume_executor,
                                                std::chrono::milliseconds ms) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    DWORD timeout = ms.count();
    const auto len = static_cast<int>(sizeof timeout);
    self.set_socket_op(SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
}

lazy_result<std::chrono::milliseconds> socket_state::send_timeout(std::shared_ptr<socket_state> self_ptr,
                                                                  std::shared_ptr<concurrencpp::executor> resume_executor) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    int timeout = 0;
    auto len = static_cast<int>(sizeof timeout);
    self.get_socket_op(SOL_SOCKET, SO_SNDTIMEO, &timeout, &len);
    co_return std::chrono::milliseconds(timeout);
}

lazy_result<void> socket_state::send_timeout(std::shared_ptr<socket_state> self_ptr,
                                             std::shared_ptr<concurrencpp::executor> resume_executor,
                                             std::chrono::milliseconds ms) {
    assert(static_cast<bool>(self_ptr));
    assert(static_cast<bool>(resume_executor));
    auto& self = *self_ptr;

    auto sg = co_await self.m_lock.lock(resume_executor);

    DWORD timeout = ms.count();
    const auto len = static_cast<int>(sizeof timeout);
    self.set_socket_op(SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
}
