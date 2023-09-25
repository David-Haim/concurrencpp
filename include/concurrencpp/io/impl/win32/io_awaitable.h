#ifndef CONCURRENCPP_IO_AWAITABLE_H
#define CONCURRENCPP_IO_AWAITABLE_H

#include "concurrencpp/io/ip.h"
#include "concurrencpp/executors/executor.h"
#include "concurrencpp/io/forward_declarations.h"
#include "concurrencpp/io/impl/win32/io_object.h"

#include <memory>
#include <optional>
#include <coroutine>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <ws2ipdef.h>
#include <Windows.h>

namespace concurrencpp::details::win32 {
    class awaitable_base : public std::suspend_always {

        /*
         *	idle ----> started -------> finished
         *   |           |                 ^ ^
         *	 |           ----> cancelled --| |
         *   |                               |
         *   --------> cancelled ------------|
         */

       public:
        class cancel_cb {

           private:
            awaitable_base& m_io_op;

           public:
            cancel_cb(awaitable_base& io_op) noexcept : m_io_op(io_op) {}

            void operator()() const noexcept {
                m_io_op.try_cancel();
            }
        };

        struct extended_overlapped : public OVERLAPPED {
            awaitable_base& io_op;

            extended_overlapped(awaitable_base& io_op) noexcept : _OVERLAPPED(), io_op(io_op) {}
        };

       protected:
        enum class status { idle, started, cancelled, finished };

        const std::shared_ptr<io_object> m_state;
        const std::shared_ptr<executor> m_resume_executor;
        io_engine& m_engine;
        concurrencpp::details::coroutine_handle<void> m_coro_handle;
        std::atomic<status> m_status {status::idle};
        extended_overlapped m_overlapped;
        std::atomic<awaitable_base*> m_next {nullptr};
        std::stop_callback<cancel_cb> m_engine_stop_cb;
        std::optional<std::stop_callback<cancel_cb>> m_user_stop_cb;
        bool m_interrupted = false;

        void cancel_impl() noexcept;
        void set_io_started() noexcept;
        void resume() noexcept;
        void try_cancel() noexcept;

       public:
        awaitable_base(std::shared_ptr<io_object> state,
                       io_engine& engine,
                       std::shared_ptr<concurrencpp::executor> resume_executor,
                       std::stop_token* optional_stop_token) noexcept;

        virtual ~awaitable_base() noexcept = default;

        virtual void start_io() noexcept = 0;
        virtual void finish_io(DWORD bytes, DWORD error_code) noexcept = 0;

        void await_suspend(concurrencpp::details::coroutine_handle<void> coroutine_handle) noexcept;

        void set_next(awaitable_base* next) noexcept;
        awaitable_base* get_next() const noexcept;

        void* handle() const noexcept;
    };

    // pass static_cast<size_t>(-1) to ignore read/write pos
    struct read_awaitable final : public awaitable_base {

       private:
        void* const m_buffer;
        const uint32_t m_buffer_length;
        uint32_t m_read = 0;
        DWORD m_error_code = 0;

       public:
        read_awaitable(std::shared_ptr<io_object> state,
                       io_engine& engine,
                       std::shared_ptr<concurrencpp::executor> resume_executor,
                       void* buffer,
                       uint32_t buffer_length,
                       size_t read_pos,
                       std::stop_token* optional_stop_token) noexcept;

        std::pair<uint32_t, DWORD> await_resume() const noexcept;

        void start_io() noexcept override;
        void finish_io(DWORD bytes, DWORD error_code) noexcept override;
    };

    struct write_awaitable final : public awaitable_base {

       private:
        const void* m_buffer;
        const uint32_t m_buffer_length;
        uint32_t m_written = 0;
        DWORD m_error_code = 0;

       public:
        write_awaitable(std::shared_ptr<io_object> state,
                        io_engine& engine,
                        std::shared_ptr<concurrencpp::executor> resume_executor,
                        const void* buffer,
                        uint32_t buffer_length,
                        size_t write_pos,
                        std::stop_token* optional_stop_token) noexcept;

        std::pair<uint32_t, DWORD> await_resume() const noexcept;

        void start_io() noexcept override;
        void finish_io(DWORD bytes, DWORD error_code) noexcept override;
    };

    struct connect_awaitable final : public awaitable_base {

       private:
        ::SOCKADDR_STORAGE m_address;
        DWORD m_error_code = 0;

       public:
        connect_awaitable(std::shared_ptr<io_object> state,
                          io_engine& engine,
                          std::shared_ptr<concurrencpp::executor> resume_executor,
                          ip_endpoint endpoint,
                          std::stop_token* optional_stop_token) noexcept;

        DWORD await_resume() const noexcept;

        void start_io() noexcept override;
        void finish_io(DWORD bytes, DWORD error_code) noexcept override;
    };

    struct accept_awaitable final : public awaitable_base {

       private:
        void* m_accept_socket_handle;
        DWORD m_error_code = 0;
        std::byte m_accept_buffer[64] = {};
        ip_endpoint m_local_endpoint;
        ip_endpoint m_remote_endpoint;

       public:
        accept_awaitable(std::shared_ptr<io_object> state,
                         io_engine& engine,
                         std::shared_ptr<concurrencpp::executor> resume_executor,
                         void* accept_socket_handle,
                         std::stop_token* optional_stop_token) noexcept;

        void start_io() noexcept override;
        void finish_io(DWORD bytes, DWORD error_code) noexcept override;

        std::tuple<ip_endpoint, ip_endpoint, DWORD> await_resume() const noexcept;
    };

    struct send_to_awaitable final : public awaitable_base {

       private:
        ::SOCKADDR_STORAGE m_addr;
        ::WSABUF m_buffer[1];
        uint32_t m_written = 0;
        DWORD m_error_code = 0;

       public:
        send_to_awaitable(std::shared_ptr<io_object> state,
                          io_engine& engine,
                          std::shared_ptr<concurrencpp::executor> resume_executor,
                          const void* buffer,
                          uint32_t buffer_length,
                          ip_endpoint endpoint,
                          std::stop_token* optional_stop_token) noexcept;

        std::pair<uint32_t, DWORD> await_resume() const noexcept;

        void start_io() noexcept override;
        void finish_io(DWORD bytes, DWORD error_code) noexcept override;
    };

    struct recv_from_awaitable final : public awaitable_base {

       private:
        ::SOCKADDR_STORAGE m_addr = {};
        ::WSABUF m_buffer[1];
        int m_addr_len = static_cast<int>(sizeof(m_addr));
        uint32_t m_read = 0;
        DWORD m_error_code = 0;

       public:
        recv_from_awaitable(std::shared_ptr<io_object> state,
                            io_engine& engine,
                            std::shared_ptr<concurrencpp::executor> resume_executor,
                            void* buffer,
                            uint32_t buffer_length,
                            std::stop_token* optional_stop_token) noexcept;

        std::tuple<uint32_t, DWORD, SOCKADDR_STORAGE> await_resume() const noexcept;

        void start_io() noexcept override;
        void finish_io(DWORD bytes, DWORD error_code) noexcept override;
    };

    // send_msg
    // recv_msg_from

}  // namespace concurrencpp::details::win32

#endif  // !CONCURRENCPP_IO_AWAITABLE_H