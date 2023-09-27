#include "concurrencpp/io/impl/win32/io_engine.h"
#include "concurrencpp/io/impl/win32/io_awaitable.h"

using concurrencpp::io_engine;
using concurrencpp::details::win32::io_object;
using concurrencpp::details::win32::awaitable_base;
using concurrencpp::details::win32::read_awaitable;
using concurrencpp::details::win32::write_awaitable;
using concurrencpp::details::win32::connect_awaitable;
using concurrencpp::details::win32::accept_awaitable;
using concurrencpp::details::win32::send_to_awaitable;
using concurrencpp::details::win32::recv_from_awaitable;

/*
 * awaitable_base
 */

awaitable_base::awaitable_base(std::shared_ptr<io_object> state,
                               io_engine& engine,
                               std::shared_ptr<concurrencpp::executor> resume_executor,
                               std::stop_token* optional_stop_token) noexcept :
    m_state(state),
    m_engine(engine), m_resume_executor(std::move(resume_executor)), m_overlapped(*this),
    m_engine_stop_cb(engine.get_stop_token(), cancel_cb(*this)) {
    if (optional_stop_token != nullptr) {
        m_user_stop_cb.emplace(*optional_stop_token, cancel_cb(*this));
    }
}

void awaitable_base::resume() noexcept {
    m_status.store(status::finished, std::memory_order_release);

    try {
        m_resume_executor->post(details::await_via_functor {m_coro_handle, &m_interrupted});
    } catch (...) {
        // do nothing. ~await_via_functor will resume the coroutine and throw an exception.
    }
}

void awaitable_base::await_suspend(concurrencpp::details::coroutine_handle<void> coroutine_handle) noexcept {
    m_coro_handle = coroutine_handle;
    m_engine.request_io_op(*this);
}

void awaitable_base::set_next(awaitable_base* next) noexcept {
    m_next.store(next, std::memory_order_release);
}

awaitable_base* awaitable_base::get_next() const noexcept {
    return m_next.load(std::memory_order_acquire);
}

void* awaitable_base::handle() const noexcept {
    return m_state->handle();
}

void awaitable_base::cancel_impl() noexcept {
    ::CancelIoEx(m_state->handle(), &m_overlapped);
}

void awaitable_base::set_io_started() noexcept {
    auto old_status = status::idle;
    if (m_status.compare_exchange_strong(old_status, status::started, std::memory_order_relaxed, std::memory_order_relaxed)) {
        return;
    }

    assert(m_status.load(std::memory_order_relaxed) == status::cancelled);
    cancel_impl();
}

void awaitable_base::try_cancel() noexcept {
    while (true) {
        auto curr_status = m_status.load(std::memory_order_relaxed);
        if (curr_status == status::cancelled) {
            return;  // someone tried to cancel us already
        }

        if (curr_status == status::finished) {
            return;  // nothing to cancel
        }

        if (curr_status == status::idle) {
            if (m_status.compare_exchange_strong(curr_status,
                                                 status::cancelled,
                                                 std::memory_order_relaxed,
                                                 std::memory_order_relaxed)) {
                return;  // will be resumed with error-cancelled by io_engine
            }
        }

        if (curr_status == status::started) {
            return cancel_impl();
        }
    }
}

/*
 * read_awaitable
 */

read_awaitable::read_awaitable(std::shared_ptr<io_object> state,
                               io_engine& engine,
                               std::shared_ptr<concurrencpp::executor> resume_executor,
                               void* buffer,
                               uint32_t buffer_length,
                               size_t read_pos,
                               std::stop_token* optional_stop_token) noexcept :
    awaitable_base(std::move(state), engine, std::move(resume_executor), optional_stop_token),
    m_buffer(buffer), m_buffer_length(buffer_length) {

    if (read_pos != static_cast<size_t>(-1)) {
        m_overlapped.Offset = static_cast<DWORD>(read_pos);
#ifdef _WIN64
        m_overlapped.OffsetHigh = static_cast<DWORD>(read_pos >> 32);
#else
        m_overlapped.OffsetHigh = 0;
#endif
    }
}

std::pair<uint32_t, DWORD> read_awaitable::await_resume() const noexcept {
    return {m_read, m_error_code};
}

void read_awaitable::start_io() noexcept {
    const auto status = m_status.load(std::memory_order_relaxed);
    assert(status != status::started);
    assert(status != status::finished);

    if (status == status::cancelled) {
        return finish_io(0, ERROR_OPERATION_ABORTED);
    }

    assert(status == status::idle);

    const auto handle = m_state->handle();
    auto res = ::ReadFile(handle, m_buffer, static_cast<DWORD>(m_buffer_length), nullptr, &m_overlapped);
    if (res == TRUE) {
        DWORD read = 0;
        DWORD error_code = 0;

        res = ::GetOverlappedResult(handle, &m_overlapped, &read, FALSE);
        if (res == 0) {
            error_code = ::GetLastError();
        }

        return finish_io(read, error_code);
    }

    if (::GetLastError() != ERROR_IO_PENDING) {
        return finish_io(0, m_error_code);
    }

    return set_io_started();
}

void read_awaitable::finish_io(DWORD bytes, DWORD error_code) noexcept {
    m_read = bytes;
    m_error_code = error_code;
    resume();
}

/*
 * write_awaitable
 */

write_awaitable::write_awaitable(std::shared_ptr<io_object> state,
                                 io_engine& engine,
                                 std::shared_ptr<concurrencpp::executor> resume_executor,
                                 const void* buffer,
                                 uint32_t buffer_length,
                                 size_t write_pos,
                                 std::stop_token* optional_stop_token) noexcept :
    awaitable_base(std::move(state), engine, std::move(resume_executor), optional_stop_token),
    m_buffer(buffer), m_buffer_length(buffer_length) {

    if (write_pos == static_cast<size_t>(-1)) {
        m_overlapped.Offset = 0xFFFFFFFF;
        m_overlapped.OffsetHigh = 0xFFFFFFFF;
    } else {
        m_overlapped.Offset = static_cast<DWORD>(write_pos);
#ifdef _WIN64
        m_overlapped.OffsetHigh = static_cast<DWORD>(write_pos >> 32);
#else
        m_overlapped.OffsetHigh = 0;
#endif
    }
}

std::pair<uint32_t, DWORD> write_awaitable::await_resume() const noexcept {
    return {m_written, m_error_code};
}

void write_awaitable::start_io() noexcept {
    const auto status = m_status.load(std::memory_order_relaxed);
    assert(status != status::started);
    assert(status != status::finished);

    if (status == status::cancelled) {
        return finish_io(0, ERROR_OPERATION_ABORTED);
    }

    assert(status == status::idle);

    const auto handle = m_state->handle();
    auto res = ::WriteFile(handle, m_buffer, m_buffer_length, nullptr, &m_overlapped);
    if (res == TRUE) {
        DWORD written = 0;
        DWORD error_code = 0;
        res = ::GetOverlappedResult(handle, &m_overlapped, &written, FALSE);
        if (res == 0) {
            error_code = ::GetLastError();
        }

        return finish_io(written, error_code);
    }

    const auto error_code = ::GetLastError();
    if (error_code != ERROR_IO_PENDING) {
        return finish_io(0, error_code);
    }

    return set_io_started();
}

void write_awaitable::finish_io(DWORD bytes, DWORD error_code) noexcept {
    m_written = bytes;
    m_error_code = error_code;
    resume();
}

/*
 * connect_awaitable
 */

connect_awaitable::connect_awaitable(std::shared_ptr<io_object> state,
                                     io_engine& engine,
                                     std::shared_ptr<concurrencpp::executor> resume_executor,
                                     ip_endpoint endpoint,
                                     std::stop_token* optional_stop_token) noexcept :
    awaitable_base(std::move(state), engine, std::move(resume_executor), optional_stop_token),
    m_address() {

    if (endpoint.address.index() == 0) {
        const auto sockaddr_ip_v4 = reinterpret_cast<::sockaddr_in*>(&m_address);
        sockaddr_ip_v4->sin_family = AF_INET;
        sockaddr_ip_v4->sin_port = ::htons(endpoint.port);
        sockaddr_ip_v4->sin_addr.S_un.S_addr = std::get<0>(endpoint.address).address();
    } else {
        const auto sockaddr_ip_v6 = reinterpret_cast<::sockaddr_in6*>(&m_address);
        sockaddr_ip_v6->sin6_family = AF_INET6;
        sockaddr_ip_v6->sin6_port = ::htons(endpoint.port);
        sockaddr_ip_v6->sin6_scope_id = std::get<1>(endpoint.address).scope_id();

        const auto address_bytes = std::get<1>(endpoint.address).bytes();
        ::memcpy_s(&sockaddr_ip_v6->sin6_addr.u.Byte[0], 16, address_bytes.data(), 16);
    }
}

DWORD connect_awaitable::await_resume() const noexcept {
    return m_error_code;
}

void connect_awaitable::start_io() noexcept {
    const auto status = m_status.load(std::memory_order_relaxed);
    assert(status != status::started);
    assert(status != status::finished);

    if (status == status::cancelled) {
        return finish_io(0, ERROR_OPERATION_ABORTED);
    }

    assert(status == status::idle);

    const auto handle = m_state->handle();
    if (m_address.ss_family == AF_INET) {
        ::sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = 0;

        // TODO: handle a case where the socket is already bound
        const auto res = ::bind(reinterpret_cast<SOCKET>(handle), reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));
        if (res != 0) {
            return finish_io(0, ::GetLastError());
        }
    } else {
        assert(m_address.ss_family == AF_INET6);
        ::sockaddr_in6 addr = {};
        addr.sin6_addr = in6addr_any;
        addr.sin6_family = AF_INET6;
        addr.sin6_port = 0;

        // TODO: handle a case where the socket us already bound
        const auto res = ::bind(reinterpret_cast<SOCKET>(handle), reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));
        if (res != 0) {
            return finish_io(0, ::GetLastError());
        }
    }

    const auto res = m_engine.get_exported_functions().ConnectEx(reinterpret_cast<SOCKET>(handle),
                                                                 reinterpret_cast<SOCKADDR*>(&m_address),
                                                                 sizeof(m_address),
                                                                 nullptr,
                                                                 0,
                                                                 nullptr,
                                                                 &m_overlapped);
    if (res == TRUE) {
        return finish_io(0, 0);
    }

    const auto error_code = ::GetLastError();
    if (error_code != ERROR_IO_PENDING) {
        return finish_io(0, error_code);
    }

    return set_io_started();
}

void connect_awaitable::finish_io(DWORD bytes, DWORD error_code) noexcept {
    if (error_code != 0) {
        m_error_code = error_code;
        return resume();
    }

    const auto handle = m_state->handle();
    const auto res = ::setsockopt(reinterpret_cast<SOCKET>(handle), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
    if (res != 0) {
        m_error_code = ::GetLastError();
    }

    return resume();
}

/*
 * accept_awaitable
 */

accept_awaitable::accept_awaitable(std::shared_ptr<io_object> state,
                                   io_engine& engine,
                                   std::shared_ptr<concurrencpp::executor> resume_executor,
                                   void* accept_socket_handle,
                                   std::stop_token* optional_stop_token) noexcept :
    awaitable_base(std::move(state), engine, std::move(resume_executor), optional_stop_token),
    m_accept_socket_handle(accept_socket_handle) {}

void accept_awaitable::start_io() noexcept {
    const auto status = m_status.load(std::memory_order_relaxed);
    assert(status != status::started);
    assert(status != status::finished);

    if (status == status::cancelled) {
        return finish_io(0, ERROR_OPERATION_ABORTED);
    }

    assert(status == status::idle);

    DWORD read = 0;
    const auto handle = m_state->handle();
    const auto res = m_engine.get_exported_functions().AcceptEx(reinterpret_cast<SOCKET>(handle),
                                                                reinterpret_cast<SOCKET>(m_accept_socket_handle),
                                                                m_accept_buffer,
                                                                0,
                                                                sizeof(m_accept_buffer) / 2,
                                                                sizeof(m_accept_buffer) / 2,
                                                                &read,
                                                                &m_overlapped);

    if (res == TRUE) {
        return finish_io(0, 0);
    }

    const auto error_code = ::GetLastError();
    if (error_code != ERROR_IO_PENDING) {
        return finish_io(0, error_code);
    }

    return set_io_started();
}

void accept_awaitable::finish_io(DWORD bytes, DWORD error_code) noexcept {
    if (error_code != 0) {
        m_error_code = error_code;
        return resume();
    }

    const auto handle = m_state->handle();
    const auto res = ::setsockopt(reinterpret_cast<SOCKET>(m_accept_socket_handle),
                                  SOL_SOCKET,
                                  SO_UPDATE_ACCEPT_CONTEXT,
                                  reinterpret_cast<const char*>(&handle),
                                  sizeof(handle));

    if (res == SOCKET_ERROR) {
        error_code = ::GetLastError();
        return resume();
    }

    void *local_address_ptr = nullptr, *remote_address_ptr = nullptr;
    INT local_len = 0, remote_len = 0;
    m_engine.get_exported_functions().GetAcceptExSockaddrs(m_accept_buffer,
                                                           0,
                                                           32,
                                                           32,
                                                           reinterpret_cast<sockaddr**>(&local_address_ptr),
                                                           &local_len,
                                                           reinterpret_cast<sockaddr**>(&remote_address_ptr),
                                                           &remote_len);

    if (error_code != 0) {
        m_error_code = error_code;
        return resume();
    }

    assert(local_len >= sizeof(sockaddr));
    auto sockaddr_ptr = static_cast<const sockaddr*>(local_address_ptr);

    if (sockaddr_ptr->sa_family == AF_INET) {
        const auto local_address = static_cast<::SOCKADDR_IN*>(local_address_ptr);
        ip_v4 local_ip(local_address->sin_addr.S_un.S_addr);
        uint16_t local_port = ::ntohs(local_address->sin_port);

        m_local_endpoint = {local_ip, local_port};

        const auto remote_address = static_cast<::SOCKADDR_IN*>(remote_address_ptr);
        ip_v4 remote_ip(remote_address->sin_addr.S_un.S_addr);
        uint16_t remote_port = ::ntohs(remote_address->sin_port);

        m_remote_endpoint = {remote_ip, remote_port};
    } else {
        assert(sockaddr_ptr->sa_family == AF_INET6);

        const auto local_address = static_cast<::SOCKADDR_IN6*>(local_address_ptr);
        const auto local_scope_id = local_address->sin6_scope_id;
        const auto local_addr_bytes = std::span<std::byte, 16>(reinterpret_cast<std::byte*>(&local_address->sin6_addr.u.Byte[0]), 16);

        ip_v6 local_ip(local_addr_bytes, local_scope_id);
        uint16_t local_port = ::ntohs(local_address->sin6_port);

        m_local_endpoint = {local_ip, local_port};

        const auto remote_address = static_cast<::SOCKADDR_IN6*>(remote_address_ptr);
        const auto remote_scope_id = remote_address->sin6_scope_id;
        const auto remote_addr_bytes =
            std::span<std::byte, 16>(reinterpret_cast<std::byte*>(&remote_address->sin6_addr.u.Byte[0]), 16);

        ip_v6 remote_ip(remote_addr_bytes, remote_scope_id);
        uint16_t remote_port = ::ntohs(remote_address->sin6_port);

        m_remote_endpoint = {remote_ip, remote_port};
    }

    resume();
}

std::tuple<concurrencpp::ip_endpoint, concurrencpp::ip_endpoint, DWORD> accept_awaitable::await_resume() const noexcept {
    return {m_local_endpoint, m_remote_endpoint, m_error_code};
}

/*
 * send_to_awaitable
 */

send_to_awaitable::send_to_awaitable(std::shared_ptr<io_object> state,
                                     io_engine& engine,
                                     std::shared_ptr<concurrencpp::executor> resume_executor,
                                     const void* buffer,
                                     uint32_t buffer_length,
                                     ip_endpoint endpoint,
                                     std::stop_token* optional_stop_token) noexcept :
    awaitable_base(std::move(state), engine, std::move(resume_executor), optional_stop_token) {
    m_buffer[0].buf = (CHAR*)buffer;
    m_buffer[0].len = buffer_length;

    if (endpoint.address.index() == 0) {
        const auto sockaddr_ip_v4 = reinterpret_cast<::sockaddr_in*>(&m_addr);
        sockaddr_ip_v4->sin_family = AF_INET;
        sockaddr_ip_v4->sin_addr.S_un.S_addr = std::get<0>(endpoint.address).address();
        sockaddr_ip_v4->sin_port = ::htons(endpoint.port);
    } else {
        const auto sockaddr_ip_v6 = reinterpret_cast<::sockaddr_in6*>(&m_addr);
        sockaddr_ip_v6->sin6_family = AF_INET6;
        sockaddr_ip_v6->sin6_port = ::htons(endpoint.port);
        sockaddr_ip_v6->sin6_scope_id = std::get<1>(endpoint.address).scope_id();

        const auto address_bytes = std::get<1>(endpoint.address).bytes();
        ::memcpy_s(&sockaddr_ip_v6->sin6_addr.u.Byte[0], 16, address_bytes.data(), 16);
    }
}

std::pair<uint32_t, DWORD> send_to_awaitable::await_resume() const noexcept {
    return {m_written, m_error_code};
}

void send_to_awaitable::start_io() noexcept {
    const auto status = m_status.load(std::memory_order_relaxed);
    assert(status != status::started);
    assert(status != status::finished);

    if (status == status::cancelled) {
        return finish_io(0, ERROR_OPERATION_ABORTED);
    }

    assert(status == status::idle);

    const auto handle = m_state->handle();
    auto res = ::WSASendTo(reinterpret_cast<SOCKET>(handle),
                           m_buffer,
                           1,
                           nullptr,
                           0,
                           (sockaddr*)&m_addr,
                           sizeof m_addr,
                           &m_overlapped,
                           nullptr);
    if (res == 0) {
        DWORD written = 0;
        DWORD error_code = 0;
        res = ::GetOverlappedResult(handle, &m_overlapped, &written, FALSE);
        if (res == 0) {
            error_code = ::GetLastError();
        }

        return finish_io(written, error_code);
    }

    const auto error_code = ::GetLastError();
    if (error_code != ERROR_IO_PENDING) {
        return finish_io(0, error_code);
    }

    return set_io_started();
}

void send_to_awaitable::finish_io(DWORD bytes, DWORD error_code) noexcept {
    m_written = bytes;
    m_error_code = error_code;
    resume();
}

/*
 * recv_from_awaitable
 */

recv_from_awaitable::recv_from_awaitable(std::shared_ptr<io_object> state,
                                         io_engine& engine,
                                         std::shared_ptr<concurrencpp::executor> resume_executor,
                                         void* buffer,
                                         uint32_t buffer_length,
                                         std::stop_token* optional_stop_token) noexcept :
    awaitable_base(std::move(state), engine, std::move(resume_executor), optional_stop_token) {
    m_buffer[0].buf = static_cast<char*>(buffer);
    m_buffer[0].len = buffer_length;
}

std::tuple<uint32_t, DWORD, SOCKADDR_STORAGE> recv_from_awaitable::await_resume() const noexcept {
    return {m_read, m_error_code, m_addr};
}

void recv_from_awaitable::start_io() noexcept {
    const auto status = m_status.load(std::memory_order_relaxed);
    assert(status != status::started);
    assert(status != status::finished);

    if (status == status::cancelled) {
        return finish_io(0, ERROR_OPERATION_ABORTED);
    }

    assert(status == status::idle);

    const auto handle = m_state->handle();
    auto res = ::WSARecvFrom(reinterpret_cast<SOCKET>(handle),
                             m_buffer,
                             1,
                             nullptr,
                             0,
                             (sockaddr*)&m_addr,
                             &m_addr_len,
                             &m_overlapped,
                             nullptr);
    if (res == 0) {
        DWORD written = 0;
        DWORD error_code = 0;
        res = ::GetOverlappedResult(handle, &m_overlapped, &written, FALSE);
        if (res == 0) {
            error_code = ::GetLastError();
        }

        return finish_io(written, error_code);
    }

    const auto error_code = ::GetLastError();
    if (error_code != ERROR_IO_PENDING) {
        return finish_io(0, error_code);
    }

    return set_io_started();
}

void recv_from_awaitable::finish_io(DWORD bytes, DWORD error_code) noexcept {
    m_read = bytes;
    m_error_code = error_code;
    resume();
}
