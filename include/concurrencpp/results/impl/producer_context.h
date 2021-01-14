#ifndef CONCURRENCPP_PRODUCER_CONTEXT_H
#define CONCURRENCPP_PRODUCER_CONTEXT_H

#include "concurrencpp/results/result_fwd_declerations.h"

#include <optional>

#include <cassert>

namespace concurrencpp::details {
    template<class type>
    class producer_context {

       private:
        std::optional<type> m_result;
        std::exception_ptr m_exception;

        void assert_state() const noexcept {
            assert((!m_result.has_value() && !static_cast<bool>(m_exception)) || (m_result.has_value() != static_cast<bool>(m_exception)));
        }

       public:
        template<class... argument_types>
        void build_result(argument_types&&... arguments) {
            assert(!m_result.has_value());
            assert(!static_cast<bool>(m_exception));
            m_result.emplace(std::forward<argument_types>(arguments)...);
        }

        void build_exception(const std::exception_ptr& exception) noexcept {
            assert(!m_result.has_value());
            assert(!static_cast<bool>(m_exception));
            m_exception = exception;
        }

        result_status status() const noexcept {
            assert_state();

            if (m_result.has_value()) {
                return result_status::value;
            }

            if (static_cast<bool>(m_exception)) {
                return result_status::exception;
            }

            return result_status::idle;
        }

        type get() {
            assert_state();

            if (m_result.has_value()) {
                return std::move(m_result.value());
            }

            assert(static_cast<bool>(m_exception));
            std::rethrow_exception(m_exception);
        }

        type& get_ref() {
            assert_state();

            if (m_result.has_value()) {
                return m_result.value();
            }

            assert(static_cast<bool>(m_exception));
            std::rethrow_exception(m_exception);
        }
    };

    template<>
    class producer_context<void> {

       private:
        std::exception_ptr m_exception;
        bool m_ready = false;

        void assert_state() const noexcept {
            assert((!m_ready && !static_cast<bool>(m_exception)) || (m_ready != static_cast<bool>(m_exception)));
        }

       public:
        void build_result() noexcept {
            assert(!m_ready);
            assert(!static_cast<bool>(m_exception));
            m_ready = true;
        }

        void build_exception(const std::exception_ptr& exception) noexcept {
            assert(!m_ready);
            assert(!static_cast<bool>(m_exception));
            m_exception = exception;
        }

        result_status status() const noexcept {
            assert_state();

            if (m_ready) {
                return result_status::value;
            }

            if (static_cast<bool>(m_exception)) {
                return result_status::exception;
            }

            return result_status::idle;
        }

        void get() const {
            assert_state();

            if (m_ready) {
                return;
            }

            assert(static_cast<bool>(m_exception));
            std::rethrow_exception(m_exception);
        }

        void get_ref() const {
            return get();
        }
    };

    template<class type>
    class producer_context<type&> {

       private:
        type* m_pointer = nullptr;
        std::exception_ptr m_exception;

        void assert_state() const noexcept {
            assert((m_pointer == nullptr && !static_cast<bool>(m_exception)) || ((m_pointer != nullptr) || static_cast<bool>(m_exception)));
        }

       public:
        void build_result(type& reference) noexcept {
            assert(m_pointer == nullptr);
            assert(!static_cast<bool>(m_exception));
            m_pointer = std::addressof(reference);
        }

        void build_exception(const std::exception_ptr& exception) noexcept {
            assert(m_pointer == nullptr);
            assert(!static_cast<bool>(m_exception));
            m_exception = exception;
        }

        result_status status() const noexcept {
            assert_state();

            if (m_pointer != nullptr) {
                return result_status::value;
            }

            if (static_cast<bool>(m_exception)) {
                return result_status::exception;
            }

            return result_status::idle;
        }

        type& get() const {
            assert_state();

            if (m_pointer != nullptr) {
                assert(reinterpret_cast<size_t>(m_pointer) % alignof(type) == 0);
                return *m_pointer;
            }

            assert(static_cast<bool>(m_exception));
            std::rethrow_exception(m_exception);
        }

        type& get_ref() const {
            return get();
        }
    };
}  // namespace concurrencpp::details

#endif