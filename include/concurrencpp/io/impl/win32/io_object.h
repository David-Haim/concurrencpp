#ifndef CONCURRENCPP_IO_IO_OBJECT_H
#define CONCURRENCPP_IO_IO_OBJECT_H

#include "concurrencpp/threads/async_lock.h"
#include "concurrencpp/io/forward_declarations.h"

namespace concurrencpp::details::win32 {
    class awaitable_base;

    class io_object {

       protected:
        using handle_cleanup_fn = void (*)(void*) noexcept;

       protected:
        std::unique_ptr<void, handle_cleanup_fn> m_handle;
        const std::weak_ptr<io_engine> m_engine;
        mutable async_lock m_lock;

        io_object(void* handle, handle_cleanup_fn cleanup_fn, std::shared_ptr<io_engine> engine) noexcept;

       public:
        void* handle() const noexcept;

        std::shared_ptr<io_engine> get_engine(const char* error_msg) const;
    };
}  // namespace concurrencpp::details::win32

#endif