#ifndef CONCURRENCPP_GENERATOR_STATE_H
#define CONCURRENCPP_GENERATOR_STATE_H

#include "concurrencpp/forward_declarations.h"
#include "concurrencpp/coroutines/coroutine.h"

namespace concurrencpp::details {
    template<typename type>
    class generator_state {

       public:
        using value_type = std::remove_reference_t<type>;

       private:
        value_type* m_value = nullptr;
        std::exception_ptr m_exception;

       public:
        generator<type> get_return_object() noexcept {
            return generator<type> {coroutine_handle<generator_state<type>>::from_promise(*this)};
        }

        suspend_always initial_suspend() const noexcept {
            return {};
        }

        suspend_always final_suspend() const noexcept {
            return {};
        }

        suspend_always yield_value(value_type& ref) noexcept {
            m_value = std::addressof(ref);
            return {};
        }

        suspend_always yield_value(value_type&& ref) noexcept {
            m_value = std::addressof(ref);
            return {};
        }

        void unhandled_exception() noexcept {
            m_exception = std::current_exception();
        }

        void return_void() const noexcept {}

        value_type& value() const noexcept {
            assert(m_value != nullptr);
            assert(reinterpret_cast<std::intptr_t>(m_value) % alignof(value_type) == 0);
            return *m_value;
        }

        void throw_if_exception() const {
            if (static_cast<bool>(m_exception)) {
                std::rethrow_exception(m_exception);
            }
        }
    };

    struct generator_end_iterator {};

    template<typename type>
    class generator_iterator {

       private:
        coroutine_handle<generator_state<type>> m_coro_handle;

       public:
        using value_type = std::remove_reference_t<type>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;

       public:
        generator_iterator(coroutine_handle<generator_state<type>> handle) noexcept : m_coro_handle(handle) {
            assert(static_cast<bool>(m_coro_handle));
        }

        generator_iterator& operator++() {
            assert(static_cast<bool>(m_coro_handle));
            assert(!m_coro_handle.done());
            m_coro_handle.resume();

            if (m_coro_handle.done()) {
                m_coro_handle.promise().throw_if_exception();
            }

            return *this;
        }

        void operator++(int) {
            (void)operator++();
        }

        reference operator*() const noexcept {
            assert(static_cast<bool>(m_coro_handle));
            return m_coro_handle.promise().value();
        }

        pointer operator->() const noexcept {
            assert(static_cast<bool>(m_coro_handle));
            return std::addressof(operator*());
        }

        friend bool operator==(const generator_iterator& it0, const generator_iterator& it1) noexcept {
            return it0.m_coro_handle == it1.m_coro_handle;
        }

        friend bool operator==(const generator_iterator& it, generator_end_iterator) noexcept {
            return it.m_coro_handle.done();
        }

        friend bool operator==(generator_end_iterator end_it, const generator_iterator& it) noexcept {
            return (it == end_it);
        }

        friend bool operator!=(const generator_iterator& it, generator_end_iterator end_it) noexcept {
            return !(it == end_it);
        }

        friend bool operator!=(generator_end_iterator end_it, const generator_iterator& it) noexcept {
            return it != end_it;
        }
    };
}  // namespace concurrencpp::details

#endif