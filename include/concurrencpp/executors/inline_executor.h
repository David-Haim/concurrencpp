#ifndef CONCURRENCPP_INLINE_EXECUTOR_H
#define CONCURRENCPP_INLINE_EXECUTOR_H

#include "concurrencpp/executors/executor.h"
#include "concurrencpp/executors/constants.h"

namespace concurrencpp {
    class CRCPP_API inline_executor final : public executor {

       private:
        std::atomic_bool m_abort;

       public:
        inline_executor() noexcept : executor(details::consts::k_inline_executor_name), m_abort(false) {}

        void enqueue(concurrencpp::task& task) override {
            if (m_abort.load(std::memory_order_relaxed)) {
                throw_runtime_shutdown_exception();
            }

            task.resume();
        }

        int max_concurrency_level() const noexcept override {
            return details::consts::k_inline_executor_max_concurrency_level;
        }

        void shutdown() override {
            m_abort.store(true, std::memory_order_relaxed);
        }

        bool shutdown_requested() const override {
            return m_abort.load(std::memory_order_relaxed);
        }

        template<class callable_type, class... argument_types>
        null_result post(callable_type&& callable, argument_types&&... arguments) {
            if (m_abort.load(std::memory_order_relaxed)) {
                throw_runtime_shutdown_exception();
            }

            try {
                callable(std::forward<argument_types>(arguments)...);            
            } catch (...) {
                // do nothing, post throws away any returned value or thrown exception
            }
   
            return {};
        }
    };
}  // namespace concurrencpp

#endif
