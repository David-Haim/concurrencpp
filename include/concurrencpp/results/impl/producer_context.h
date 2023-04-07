#ifndef CONCURRENCPP_PRODUCER_CONTEXT_H
#define CONCURRENCPP_PRODUCER_CONTEXT_H

#include "concurrencpp/results/result_fwd_declarations.h"

#include <exception>

#include <cassert>

namespace concurrencpp::details {
    template<class type>
    class producer_context {

        union storage {
            type object;
            std::exception_ptr exception;

            storage() noexcept {}
            ~storage() noexcept {}
        };

       private:
        storage m_storage;
        result_status m_status = result_status::idle;

       public:
        ~producer_context() noexcept {
            switch (m_status) {
                case result_status::value: {
                    m_storage.object.~type();
                    break;
                }

                case result_status::exception: {
                    m_storage.exception.~exception_ptr();
                    break;
                }

                case result_status::idle: {
                    break;
                }

                default: {
                    assert(false);
                }
            }
        }

        template<class... argument_types>
        void build_result(argument_types&&... arguments) noexcept(noexcept(type(std::forward<argument_types>(arguments)...))) {
            assert(m_status == result_status::idle);
            new (std::addressof(m_storage.object)) type(std::forward<argument_types>(arguments)...);
            m_status = result_status::value;
        }

        void build_exception(const std::exception_ptr& exception) noexcept {
            assert(m_status == result_status::idle);
            new (std::addressof(m_storage.exception)) std::exception_ptr(exception);
            m_status = result_status::exception;
        }

        result_status status() const noexcept {
            return m_status;
        }

        type get() {
            return std::move(get_ref());
        }

        type& get_ref() {
            assert(m_status != result_status::idle);
            if (m_status == result_status::value) {
                return m_storage.object;
            }

            assert(m_status == result_status::exception);
            assert(static_cast<bool>(m_storage.exception));
            std::rethrow_exception(m_storage.exception);
        }
    };

    template<>
    class producer_context<void> {

        union storage {
            std::exception_ptr exception;

            storage() noexcept {}
            ~storage() noexcept {}
        };

       private:
        storage m_storage;
        result_status m_status = result_status::idle;

       public:
        ~producer_context() noexcept {
            if (m_status == result_status::exception) {
                m_storage.exception.~exception_ptr();
            }
        }

        producer_context& operator=(producer_context&& rhs) noexcept {
            assert(m_status == result_status::idle);
            m_status = std::exchange(rhs.m_status, result_status::idle);

            if (m_status == result_status::exception) {
                new (std::addressof(m_storage.exception)) std::exception_ptr(rhs.m_storage.exception);
                rhs.m_storage.exception.~exception_ptr();
            }

            return *this;
        }

        void build_result() noexcept {
            assert(m_status == result_status::idle);
            m_status = result_status::value;
        }

        void build_exception(const std::exception_ptr& exception) noexcept {
            assert(m_status == result_status::idle);
            new (std::addressof(m_storage.exception)) std::exception_ptr(exception);
            m_status = result_status::exception;
        }

        result_status status() const noexcept {
            return m_status;
        }

        void get() const {
            get_ref();
        }

        void get_ref() const {
            assert(m_status != result_status::idle);
            if (m_status == result_status::exception) {
                assert(static_cast<bool>(m_storage.exception));
                std::rethrow_exception(m_storage.exception);
            }
        }
    };

    template<class type>
    class producer_context<type&> {

        union storage {
            type* pointer;
            std::exception_ptr exception;

            storage() noexcept {}
            ~storage() noexcept {}
        };

       private:
        storage m_storage;
        result_status m_status = result_status::idle;

       public:
        ~producer_context() noexcept {
            if (m_status == result_status::exception) {
                m_storage.exception.~exception_ptr();
            }
        }

        producer_context& operator=(producer_context&& rhs) noexcept {
            assert(m_status == result_status::idle);
            m_status = std::exchange(rhs.m_status, result_status::idle);

            switch (m_status) {
                case result_status::value: {
                    m_storage.pointer = rhs.m_storage.pointer;
                    break;
                }

                case result_status::exception: {
                    new (std::addressof(m_storage.exception)) std::exception_ptr(rhs.m_storage.exception);
                    rhs.m_storage.exception.~exception_ptr();
                    break;
                }

                case result_status::idle: {
                    break;
                }

                default: {
                    assert(false);
                }
            }

            return *this;
        }

        void build_result(type& reference) noexcept {
            assert(m_status == result_status::idle);

            auto pointer = std::addressof(reference);
            assert(pointer != nullptr);
            assert(reinterpret_cast<size_t>(pointer) % alignof(type) == 0);

            m_storage.pointer = pointer;
            m_status = result_status::value;
        }

        void build_exception(const std::exception_ptr& exception) noexcept {
            assert(m_status == result_status::idle);
            new (std::addressof(m_storage.exception)) std::exception_ptr(exception);
            m_status = result_status::exception;
        }

        result_status status() const noexcept {
            return m_status;
        }

        type& get() const {
            return get_ref();
        }

        type& get_ref() const {
            assert(m_status != result_status::idle);

            if (m_status == result_status::value) {
                assert(m_storage.pointer != nullptr);
                assert(reinterpret_cast<size_t>(m_storage.pointer) % alignof(type) == 0);
                return *m_storage.pointer;
            }

            assert(m_status == result_status::exception);
            assert(static_cast<bool>(m_storage.exception));
            std::rethrow_exception(m_storage.exception);
        }
    };
}  // namespace concurrencpp::details

#endif