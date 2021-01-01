#ifndef CONCURRENCPP_RESULT_STATE_H
#define CONCURRENCPP_RESULT_STATE_H

#include "concurrencpp/results/impl/consumer_context.h"
#include "concurrencpp/results/impl/producer_context.h"

#include <atomic>
#include <type_traits>

#include <cassert>

namespace concurrencpp::details {
    class result_state_base {

       public:
        enum class pc_state { idle, producer, consumer };

       private:
        bool await_via_ready(await_via_context& await_ctx, bool force_rescheduling) noexcept;

       protected:
        std::atomic<pc_state> m_pc_state;
        consumer_context m_consumer;

        void assert_done() const noexcept;

       public:
        void wait();

        bool await(await_context& await_ctx) noexcept;

        bool await_via(await_via_context& await_ctx, bool force_rescheduling) noexcept;

        void when_all(std::shared_ptr<when_all_state_base> when_all_state) noexcept;

        when_any_status when_any(std::shared_ptr<when_any_state_base> when_any_state, size_t index) noexcept;

        void try_rewind_consumer() noexcept;

        void publish_result() noexcept;
    };

    template<class type>
    class result_state : public result_state_base {

       private:
        producer_context<type> m_producer;

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
        void set_result(argument_types&&... arguments) {
            m_producer.build_result(std::forward<argument_types>(arguments)...);
        }

        void set_exception(std::exception_ptr error) noexcept {
            assert(error != nullptr);
            m_producer.build_exception(std::move(error));
        }

        // Consumer-side functions
        result_status status() const noexcept {
            const auto state = m_pc_state.load(std::memory_order_acquire);
            assert(state != pc_state::consumer);

            if (state == pc_state::idle) {
                return result_status::idle;
            }

            return m_producer.status();
        }

        template<class duration_unit, class ratio>
        concurrencpp::result_status wait_for(std::chrono::duration<duration_unit, ratio> duration) {
            const auto state_0 = m_pc_state.load(std::memory_order_acquire);
            if (state_0 == pc_state::producer) {
                return m_producer.status();
            }

            auto wait_ctx = std::make_shared<wait_context>();
            m_consumer.set_wait_context(wait_ctx);

            auto expected_idle_state = pc_state::idle;
            const auto idle_0 = m_pc_state.compare_exchange_strong(expected_idle_state, pc_state::consumer, std::memory_order_acq_rel);

            if (!idle_0) {
                assert_done();
                return m_producer.status();
            }

            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            if (wait_ctx->wait_for(static_cast<size_t>(ms + 1))) {
                assert_done();
                return m_producer.status();
            }

            /*
                now we need to rewind what we've done: the producer might try to
                access the consumer context while we rewind the consumer context back to
                nothing. first we'll cas the status back to idle. if we failed - the
                producer has set its result, then there's no point in continue rewinding
                - we just return the status of the result. if we managed to rewind the
                status back to idle, then the consumer is "protected" because the
                producer will not try to access the consumer if the flag doesn't say so.
            */
            auto expected_consumer_state = pc_state::consumer;
            const auto idle_1 = m_pc_state.compare_exchange_strong(expected_consumer_state, pc_state::idle, std::memory_order_acq_rel);

            if (!idle_1) {
                assert_done();
                return m_producer.status();
            }

            m_consumer.clear();
            return result_status::idle;
        }

        template<class clock, class duration>
        concurrencpp::result_status wait_until(const std::chrono::time_point<clock, duration>& timeout_time) {
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

        template<class callable_type>
        void from_callable(callable_type&& callable) noexcept {
            using is_void = std::is_same<type, void>;

            try {
                from_callable(is_void {}, std::forward<callable_type>(callable));
            } catch (...) {
                set_exception(std::current_exception());
            }
        }
    };
}  // namespace concurrencpp::details

#endif
