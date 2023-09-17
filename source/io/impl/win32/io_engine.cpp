#include "concurrencpp/errors.h"
#include "concurrencpp/io/constants.h"
#include "concurrencpp/io/impl/win32/utils.h"
#include "concurrencpp/io/impl/win32/io_engine.h"

#include <cassert>

#pragma comment(lib, "ws2_32.lib")

namespace concurrencpp::details::win32 {
    template<class function_type>
    static void export_wsa_function(SOCKET socket, GUID function_guid, function_type& out_function_ptr) {
        DWORD unused = 0;
        const auto res = ::WSAIoctl(socket,
                                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                                    &function_guid,
                                    sizeof(function_guid),
                                    &out_function_ptr,
                                    sizeof(function_type),
                                    &unused,
                                    nullptr,
                                    nullptr);

        if (res == SOCKET_ERROR) {
            throw_system_error(::WSAGetLastError());
        }

        assert(out_function_ptr != nullptr);
    }

    struct iocp_constants {
        constexpr static size_t k_iocp_completion_key = 0x123456789;
        constexpr static LPOVERLAPPED k_stop_requested = reinterpret_cast<LPOVERLAPPED>(alignof(OVERLAPPED));
        constexpr static LPOVERLAPPED k_io_requested = reinterpret_cast<LPOVERLAPPED>(alignof(OVERLAPPED) * 2);
    };
}  // namespace concurrencpp::details::win32

using concurrencpp::io_engine;
using concurrencpp::details::win32::awaitable_base;
using concurrencpp::details::win32::iocp_constants;

io_engine::wsa_exported_functions::wsa_exported_functions() {
    const auto sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        details::throw_system_error(::WSAGetLastError());
    }

    struct socket_deleter {
        using pointer = SOCKET;

        void operator()(SOCKET socket) const noexcept {
            if (socket != INVALID_SOCKET) {
                ::closesocket(socket);
            }
        }
    };

    std::unique_ptr<SOCKET, socket_deleter> raii_guard(sock, socket_deleter {});

    details::win32::export_wsa_function(sock, WSAID_ACCEPTEX, AcceptEx);
    details::win32::export_wsa_function(sock, WSAID_CONNECTEX, ConnectEx);
    details::win32::export_wsa_function(sock, WSAID_GETACCEPTEXSOCKADDRS, GetAcceptExSockaddrs);
}

io_engine::io_engine() {
    ::WSADATA wsd = {};
    const auto res = ::WSAStartup(MAKEWORD(2, 2), &wsd);
    if (res != 0) {
        details::throw_system_error(res);
    }

    m_wsa_exported_functions = std::make_unique<wsa_exported_functions>();

    m_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, iocp_constants::k_iocp_completion_key, 1);
    if (m_iocp_handle == nullptr) {
        details::throw_system_error(::GetLastError());
    }

    m_iocp_thread = std::thread([this] {
        iocp_work_loop();
    });
}

io_engine::~io_engine() noexcept {
    const auto res = ::CloseHandle(m_iocp_handle);
    assert(res);
    ::WSACleanup();
}

void io_engine::shutdown() noexcept {
    const auto stopped_before = m_abort.exchange(true, std::memory_order_acq_rel);
    if (stopped_before) {
        return;
    }

    const auto res =
        ::PostQueuedCompletionStatus(m_iocp_handle, 0, iocp_constants::k_iocp_completion_key, iocp_constants::k_stop_requested);
    assert(res != 0);

    m_iocp_thread.join();
}

awaitable_base* io_engine::flush_request_queue() noexcept {
    const auto head = m_request_queue.exchange(nullptr, std::memory_order_acq_rel);
    awaitable_base* current = head;
    awaitable_base* prev = nullptr;

    while (current != nullptr) {
        const auto next = current->get_next();
        current->set_next(prev);
        prev = current;
        current = next;
    }

    return prev;
}

void io_engine::process_request_queue() noexcept {
    auto op_head = flush_request_queue();

    while (op_head != nullptr) {
        const auto next = op_head->get_next();
        op_head->start_io();
        op_head = next;
    }
}

void io_engine::iocp_work_loop() noexcept {
    std::vector<OVERLAPPED_ENTRY> overlapped_entries(4 * 1024);

    while (true) {
        ULONG entries_removed = 0;
        bool process_new_io_ops = false;
        bool shutdown_requested = false;
        const auto res = ::GetQueuedCompletionStatusEx(m_iocp_handle,
                                                       overlapped_entries.data(),
                                                       overlapped_entries.size(),
                                                       &entries_removed,
                                                       INFINITE,
                                                       FALSE);

        assert(res != FALSE);

        for (ULONG i = 0; i < entries_removed; i++) {
            const auto& entry = overlapped_entries[i];

            assert(entry.lpCompletionKey == iocp_constants::k_iocp_completion_key);

            if (entry.lpOverlapped == iocp_constants::k_stop_requested) {
                shutdown_requested = true;
                continue;
            }

            if (entry.lpOverlapped == iocp_constants::k_io_requested) {
                process_new_io_ops = true;
                continue;
            }

            if (entry.lpOverlapped == nullptr) {
                continue;
            }

            auto* eo = static_cast<awaitable_base::extended_overlapped*>(entry.lpOverlapped);
            auto& io_op = eo->io_op;
            const auto byted_transferred = entry.dwNumberOfBytesTransferred;
            DWORD error_code = 0;

            if (entry.lpOverlapped->Internal != 0) {  // semi-documented, but good enough - Internal is the NTStatus of the op
                DWORD bt = 0;
                const auto io_res = ::GetOverlappedResultEx(io_op.handle(), entry.lpOverlapped, &bt, 0, FALSE);
                assert(io_res != ERROR_IO_INCOMPLETE);
                assert(io_res != WAIT_IO_COMPLETION);
                assert(io_res != WAIT_TIMEOUT);

                error_code = ::GetLastError();
            }

            io_op.finish_io(byted_transferred, error_code);
        }

        if (shutdown_requested) {
            return shutdown_impl();
        }

        if (process_new_io_ops) {
            process_request_queue();
        }
    }
}

void io_engine::shutdown_impl() noexcept {
    // cancel pending io
    m_stop_source.request_stop();

    // give the os time to cancel all pending io ops
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    // poll all cancelled ops and finish them gracefully with an error
    while (true) {
        DWORD bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        LPOVERLAPPED overlapped = nullptr;
        DWORD error_code = 0;
        const auto res = ::GetQueuedCompletionStatus(m_iocp_handle, &bytes_transferred, &completion_key, &overlapped, 0);

        if (res == FALSE) {
            if (overlapped == nullptr) {
                break;
            }

            error_code = ::GetLastError();
        }

        auto* eo = static_cast<awaitable_base::extended_overlapped*>(overlapped);
        eo->io_op.finish_io(bytes_transferred, error_code);
    }

    // cancel pending requests
    auto head_op = flush_request_queue();
    while (head_op != nullptr) {
        const auto next = head_op;
        head_op->finish_io(0, ERROR_OPERATION_ABORTED);
        head_op = next;
    }
}

void io_engine::throw_if_aborted() const {
    if (m_abort.load(std::memory_order_relaxed)) {
        throw errors::runtime_shutdown(details::consts::k_io_engine_shutdown_err_msg);
    }
}

io_engine::wsa_exported_functions& io_engine::get_exported_functions() noexcept {
    assert(static_cast<bool>(m_wsa_exported_functions));
    return *m_wsa_exported_functions;
}

void io_engine::register_handle(void* io_object) const {
    throw_if_aborted();

    // see https://devblogs.microsoft.com/oldnewthing/20190719-00/?p=102722
    const auto res =
        ::SetFileCompletionNotificationModes(io_object, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS | FILE_SKIP_SET_EVENT_ON_HANDLE);
    if (res == 0) {
        details::throw_system_error(::GetLastError());
    }

    const auto handle = ::CreateIoCompletionPort(io_object, m_iocp_handle, iocp_constants::k_iocp_completion_key, 0);
    if (handle == nullptr) {
        details::throw_system_error(::GetLastError());
    }
}

void io_engine::request_io_op(awaitable_base& io_op) noexcept {
    throw_if_aborted();

    auto empty = false;
    while (true) {
        auto tail_op = m_request_queue.load(std::memory_order_acquire);
        empty = (tail_op == nullptr);

        io_op.set_next(tail_op);

        if (m_request_queue.compare_exchange_weak(tail_op, &io_op, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            break;
        }
    }

    if (empty) {
        const auto res =
            ::PostQueuedCompletionStatus(m_iocp_handle, 0, iocp_constants::k_iocp_completion_key, iocp_constants::k_io_requested);
        assert(res != 0);  // we shouldn't fail posting a packet if the machine has memory and we didn't corrupt the IOCP internals
    }
}

std::stop_token io_engine::get_stop_token() const noexcept {
    return m_stop_source.get_token();
}
