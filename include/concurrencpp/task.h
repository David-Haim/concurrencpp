#ifndef CONCURRENCPP_TASK_H
#define CONCURRENCPP_TASK_H

#include "concurrencpp/coroutines/coroutine.h"

#include <type_traits>
#include <utility>

#include <cstddef>
#include <cassert>

namespace concurrencpp::details {
    struct task_constants {
        static constexpr size_t total_size = 64;
        static constexpr size_t buffer_size = total_size - sizeof(void*);
    };

    struct vtable {
        void (*move_destroy_fn)(void* src, void* dst) noexcept;
        void (*execute_destroy_fn)(void* target);
        void (*destroy_fn)(void* target) noexcept;

        vtable(const vtable&) noexcept = default;

        constexpr vtable() noexcept : move_destroy_fn(nullptr), execute_destroy_fn(nullptr), destroy_fn(nullptr) {}

        constexpr vtable(decltype(move_destroy_fn) move_destroy_fn,
                         decltype(execute_destroy_fn) execute_destroy_fn,
                         decltype(destroy_fn) destroy_fn) noexcept :
            move_destroy_fn(move_destroy_fn),
            execute_destroy_fn(execute_destroy_fn), destroy_fn(destroy_fn) {}

        static constexpr bool trivially_copiable_destructible(decltype(move_destroy_fn) move_fn) noexcept {
            return move_fn == nullptr;
        }

        static constexpr bool trivially_destructible(decltype(destroy_fn) destroy_fn) noexcept {
            return destroy_fn == nullptr;
        }
    };

    template<class callable_type>
    class callable_vtable {

       private:
        static callable_type* inline_ptr(void* src) noexcept {
            return static_cast<callable_type*>(src);
        }

        static callable_type* allocated_ptr(void* src) noexcept {
            return *static_cast<callable_type**>(src);
        }

        static callable_type*& allocated_ref_ptr(void* src) noexcept {
            return *static_cast<callable_type**>(src);
        }

        static void move_destroy_inline(void* src, void* dst) noexcept {
            auto callable_ptr = inline_ptr(src);
            new (dst) callable_type(std::move(*callable_ptr));
            callable_ptr->~callable_type();
        }

        static void move_destroy_allocated(void* src, void* dst) noexcept {
            auto callable_ptr = std::exchange(allocated_ref_ptr(src), nullptr);
            new (dst) callable_type*(callable_ptr);
        }

        static void execute_destroy_inline(void* target) {
            auto callable_ptr = inline_ptr(target);
            (*callable_ptr)();
            callable_ptr->~callable_type();
        }

        static void execute_destroy_allocated(void* target) {
            auto callable_ptr = allocated_ptr(target);
            (*callable_ptr)();
            delete callable_ptr;
        }

        static void destroy_inline(void* target) noexcept {
            auto callable_ptr = inline_ptr(target);
            callable_ptr->~callable_type();
        }

        static void destroy_allocated(void* target) noexcept {
            auto callable_ptr = allocated_ptr(target);
            delete callable_ptr;
        }

        static constexpr vtable make_vtable() noexcept {
            void (*move_destroy_fn)(void* src, void* dst) noexcept = nullptr;
            void (*destroy_fn)(void* target) noexcept = nullptr;

            if constexpr (std::is_trivially_copy_constructible_v<callable_type> && std::is_trivially_destructible_v<callable_type> &&
                          is_inlinable()) {
                move_destroy_fn = nullptr;
            } else {
                move_destroy_fn = move_destroy;
            }

            if constexpr (std::is_trivially_destructible_v<callable_type> && is_inlinable()) {
                destroy_fn = nullptr;
            } else {
                destroy_fn = destroy;
            }

            return vtable(move_destroy_fn, execute_destroy, destroy_fn);
        }

        template<class passed_callable_type>
        static void build_inlinable(void* dst, passed_callable_type&& callable) {
            new (dst) callable_type(std::forward<passed_callable_type>(callable));
        }

        template<class passed_callable_type>
        static void build_allocated(void* dst, passed_callable_type&& callable) {
            auto new_ptr = new callable_type(std::forward<passed_callable_type>(callable));
            new (dst) callable_type*(new_ptr);
        }

       public:
        static constexpr bool is_inlinable() noexcept {
            return std::is_nothrow_move_constructible_v<callable_type> && sizeof(callable_type) <= task_constants::buffer_size;
        }

        template<class passed_callable_type>
        static void build(void* dst, passed_callable_type&& callable) {
            if (is_inlinable()) {
                return build_inlinable(dst, std::forward<passed_callable_type>(callable));
            }

            build_allocated(dst, std::forward<passed_callable_type>(callable));
        }

        static void move_destroy(void* src, void* dst) noexcept {
            assert(src != nullptr);
            assert(dst != nullptr);

            if (is_inlinable()) {
                return move_destroy_inline(src, dst);
            }

            return move_destroy_allocated(src, dst);
        }

        static void execute_destroy(void* target) {
            assert(target != nullptr);

            if (is_inlinable()) {
                return execute_destroy_inline(target);
            }

            return execute_destroy_allocated(target);
        }

        static void destroy(void* target) noexcept {
            assert(target != nullptr);

            if (is_inlinable()) {
                return destroy_inline(target);
            }

            return destroy_allocated(target);
        }

        static constexpr callable_type* as(void* src) noexcept {
            if (is_inlinable()) {
                return inline_ptr(src);
            }

            return allocated_ptr(src);
        }

        static constexpr inline vtable s_vtable = make_vtable();
    };

}  // namespace concurrencpp::details

namespace concurrencpp {
    class CRCPP_API task {

       private:
        alignas(std::max_align_t) std::byte m_buffer[details::task_constants::buffer_size];
        const details::vtable* m_vtable;

        void build(task&& rhs) noexcept;
        void build(details::coroutine_handle<void> coro_handle) noexcept;

        template<class callable_type>
        void build(callable_type&& callable) {
            using decayed_type = typename std::decay_t<callable_type>;

            details::callable_vtable<decayed_type>::build(m_buffer, std::forward<callable_type>(callable));
            m_vtable = &details::callable_vtable<decayed_type>::s_vtable;
        }

        template<class callable_type>
        static bool contains(const details::vtable* const vtable) noexcept {
            return vtable == &details::callable_vtable<callable_type>::s_vtable;
        }

        bool contains_coroutine_handle() const noexcept;

       public:
        task() noexcept;
        task(task&& rhs) noexcept;
        task(details::coroutine_handle<void> coro_handle) noexcept;

        template<class callable_type>
        task(callable_type&& callable) {
            build(std::forward<callable_type>(callable));
        }

        ~task() noexcept;

        task(const task& rhs) = delete;
        task& operator=(const task&& rhs) = delete;

        void operator()();

        task& operator=(task&& rhs) noexcept;

        void clear() noexcept;

        explicit operator bool() const noexcept;

        template<class callable_type>
        bool contains() const noexcept {
            using decayed_type = typename std::decay_t<callable_type>;

            if constexpr (std::is_same_v<decayed_type, details::coroutine_handle<void>>) {
                return contains_coroutine_handle();
            }

            return m_vtable == &details::callable_vtable<decayed_type>::s_vtable;
        }
    };
}  // namespace concurrencpp

#endif
