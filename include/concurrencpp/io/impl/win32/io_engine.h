#ifndef ASYNCIO_WIN32_IO_ENGINE_H
#define ASYNCIO_WIN32_IO_ENGINE_H

#include "concurrencpp/io/ip.h"
#include "concurrencpp/io/forward_declarations.h"
#include "concurrencpp/io/impl/win32/io_awaitable.h"

#include <thread>
#include <stop_token>

#include <cstdint>

#define NOMINMAX
#include <mswsock.h>

namespace concurrencpp {
    class io_engine {

       public:
           //TODO: move to a different file that will be included only in .cpp 
        struct wsa_exported_functions {
            ::LPFN_ACCEPTEX AcceptEx = nullptr;
            ::LPFN_CONNECTEX ConnectEx = nullptr;
            ::LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs = nullptr;

            wsa_exported_functions();
        };

       private:
        void* m_iocp_handle;
        std::unique_ptr<wsa_exported_functions> m_wsa_exported_functions;
        std::stop_source m_stop_source;
        std::thread m_iocp_thread;
        alignas(64) std::atomic<details::win32::awaitable_base*> m_request_queue {nullptr};
        alignas(64) std::atomic_bool m_abort {false};

        details::win32::awaitable_base* flush_request_queue() noexcept;

        void process_request_queue() noexcept;

        void iocp_work_loop() noexcept;

        void shutdown_impl() noexcept;

        void throw_if_aborted() const;

       public:
        io_engine();
        ~io_engine() noexcept;

        void shutdown() noexcept;

        wsa_exported_functions& get_exported_functions() noexcept;

        void register_handle(void* io_object) const;

        void request_io_op(details::win32::awaitable_base& io_op) noexcept;

        std::stop_token get_stop_token() const noexcept;
    };
}

#endif