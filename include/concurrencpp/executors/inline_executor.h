#ifndef CONCURRENCPP_INLINE_EXECUTOR_H
#define CONCURRENCPP_INLINE_EXECUTOR_H

#include "concurrencpp/executors/executor.h"
#include "concurrencpp/executors/constants.h"

namespace concurrencpp {
    class CRCPP_API inline_executor final : public executor {

       private:
        std::atomic_bool m_abort;

        void throw_if_aborted() const {
            if (m_abort.load(std::memory_order_relaxed)) {
                details::throw_runtime_shutdown_exception(name);
            }
        }

       public:
        inline_executor() noexcept : executor(details::consts::k_inline_executor_name), m_abort(false) {}

        void enqueue(concurrencpp::task task) override {
            throw_if_aborted();
            task();
        }

        void enqueue(std::span<concurrencpp::task> tasks) override {
            throw_if_aborted();
            for (auto& task : tasks) {
                task();
            }
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
    };
}  // namespace concurrencpp

#endif
