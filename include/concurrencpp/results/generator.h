#ifndef CONCURRENCPP_GENERATOR_H
#define CONCURRENCPP_GENERATOR_H

#include "concurrencpp/results/constants.h"
#include "concurrencpp/results/impl/generator_state.h"

namespace concurrencpp {
    template<typename type>
    class generator {

       public:
        using promise_type = details::generator_state<type>;
        using iterator = details::generator_iterator<type>;

        static_assert(!std::is_same_v<type, void>, "concurrencpp::generator<type> - <<type>> can not be void.");

       private:
        details::coroutine_handle<promise_type> m_coro_handle;

       public:
        generator(details::coroutine_handle<promise_type> handle) noexcept : m_coro_handle(handle) {}

        generator(generator&& rhs) noexcept : m_coro_handle(std::exchange(rhs.m_coro_handle, {})) {}

        ~generator() noexcept {
            if (static_cast<bool>(m_coro_handle)) {
                m_coro_handle.destroy();
            }
        }

        generator(const generator& rhs) = delete;

        generator& operator=(generator&& rhs) = delete;
        generator& operator=(const generator& rhs) = delete;

        explicit operator bool() const noexcept {
            return static_cast<bool>(m_coro_handle);
        }

        iterator begin() {
            if (!static_cast<bool>(m_coro_handle)) {
                throw errors::empty_generator(details::consts::k_empty_generator_begin_err_msg);
            }

            assert(!m_coro_handle.done());
            m_coro_handle.resume();

            if (m_coro_handle.done()) {
                m_coro_handle.promise().throw_if_exception();
            }

            return iterator {m_coro_handle};
        }

        static details::generator_end_iterator end() noexcept {
            return {};
        }
    };
}  // namespace concurrencpp

#endif