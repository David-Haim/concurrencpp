#ifndef CONCURRENCPP_RESULT_STATE_H
#define CONCURRENCPP_RESULT_STATE_H

#include "concurrencpp/threads/atomic_wait.h"
#include "concurrencpp/results/impl/consumer_context.h"
#include "concurrencpp/results/impl/producer_context.h"
#include "concurrencpp/platform_defs.h"

#include <atomic>
#include <type_traits>

#include <cassert>

namespace concurrencpp::details {
    class CRCPP_API result_state_base {

       public:
        enum class pc_status : uint32_t { idle, consumer_set, consumer_waiting, consumer_done, producer_done };

       protected:
        std::atomic<pc_status> m_pc_status {pc_status::idle};
        consumer_context m_consumer;
        coroutine_handle<void> m_done_handle;

        void assert_done() const noexcept;

       public:
        void wait() noexcept;
        pc_status wait_for(std::chrono::milliseconds ms) noexcept;
        bool await(coroutine_handle<void> caller_handle) noexcept;
        pc_status when_any(const std::shared_ptr<when_any_context>& when_any_state) noexcept;

        void share(const std::shared_ptr<shared_result_state_base>& shared_result_state) noexcept;

        void try_rewind_consumer() noexcept;
    };

    template<class type>
    class result_state : public result_state_base {

       private:
        producer_context<type> m_producer;

        static void delete_self(result_state<type>* state) noexcept {
            assert(state != nullptr);

            auto done_handle = state->m_done_handle;
            if (static_cast<bool>(done_handle)) {
                assert(done_handle.done());
                return done_handle.destroy();
            }

#ifdef CRCPP_GCC_COMPILER
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
            delete state;
#ifdef CRCPP_GCC_COMPILER
#    pragma GCC diagnostic pop
#endif
        }

        template<class callable_type>
        void from_callable(std::true_type /*is_void_type*/, callable_type&& callable) {
            callable();
            set_result();
        }

        template<class callable_type>
        void from_callable(std::false_type /*is_void_type*/, callable_type&& callable) {
            set_result(callable());
        }

       public:
        template<class... argument_types>
        void set_result(argument_types&&... arguments) noexcept(noexcept(type(std::forward<argument_types>(arguments)...))) {
            m_producer.build_result(std::forward<argument_types>(arguments)...);
        }

        void set_exception(const std::exception_ptr& error) noexcept {
            assert(error != nullptr);
            m_producer.build_exception(error);
        }

        // Consumer-side functions
        result_status status() const noexcept {
            const auto status = m_pc_status.load(std::memory_order_acquire);
            assert(status != pc_status::consumer_set);

            if (status == pc_status::idle) {
                return result_status::idle;
            }

            return m_producer.status();
        }

        template<class duration_unit, class ratio>
        result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) noexcept {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            const auto status = result_state_base::wait_for(ms);
            if (status == pc_status::producer_done) {
                return m_producer.status();
            }

            return result_status::idle;
        }

        template<class clock, class duration>
        result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
            const auto now = clock::now();
            if (timeout_time <= now) {
                return status();
            }

            const auto diff = timeout_time - now;
            return wait_for(diff);
        }

        type get() {
            assert_done();
            return m_producer.get();
        }

        std::add_lvalue_reference_t<type> get_ref() {
            assert_done();
            return m_producer.get_ref();
        }

        template<class callable_type>
        void from_callable(callable_type&& callable) noexcept {
            using is_void = std::is_same<type, void>;

            try {
                from_callable(is_void {}, std::forward<callable_type>(callable));
            } catch (...) {
                set_exception(std::current_exception());
            }
        }

        void complete_producer(coroutine_handle<void> done_handle = {}) noexcept {
            m_done_handle = done_handle;

            const auto status = m_pc_status.exchange(pc_status::producer_done, std::memory_order_acq_rel);
            assert(status != pc_status::producer_done);

            switch (status) {
                case pc_status::consumer_set: {
                    return m_consumer.resume_consumer(*this);
                }

                case pc_status::idle: {
                    return;
                }

                case pc_status::consumer_waiting: {
                    return atomic_notify_one(m_pc_status);
                }

                case pc_status::consumer_done: {
                    return delete_self(this);
                }

                default: {
                    break;
                }
            }

            assert(false);
        }

        void complete_consumer() noexcept {
            const auto status = m_pc_status.exchange(pc_status::consumer_done, std::memory_order_acq_rel);
            assert(status != pc_status::consumer_set);

            if (status == pc_status::producer_done) {
                return delete_self(this);
            }

            assert(status == pc_status::idle);
        }

        void complete_joined_consumer() noexcept {
            assert_done();
            delete_self(this);
        }
    };

    template<class type>
    struct consumer_result_state_deleter {
        void operator()(result_state<type>* state_ptr) const noexcept {
            assert(state_ptr != nullptr);
            state_ptr->complete_consumer();
        }
    };

    template<class type>
    struct joined_consumer_result_state_deleter {
        void operator()(result_state<type>* state_ptr) const noexcept {
            assert(state_ptr != nullptr);
            state_ptr->complete_joined_consumer();
        }
    };

    template<class type>
    struct producer_result_state_deleter {
        void operator()(result_state<type>* state_ptr) const {
            assert(state_ptr != nullptr);
            state_ptr->complete_producer();
        }
    };

    template<class type>
    using consumer_result_state_ptr = std::unique_ptr<result_state<type>, consumer_result_state_deleter<type>>;

    template<class type>
    using joined_consumer_result_state_ptr = std::unique_ptr<result_state<type>, joined_consumer_result_state_deleter<type>>;

    template<class type>
    using producer_result_state_ptr = std::unique_ptr<result_state<type>, producer_result_state_deleter<type>>;
}  // namespace concurrencpp::details

#endif
