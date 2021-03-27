#ifndef CONCURRENCPP_RESULT_TEST_EXECUTORS_H
#define CONCURRENCPP_RESULT_TEST_EXECUTORS_H

#include "concurrencpp/executors/executor.h"

namespace concurrencpp::tests {
    struct executor_enqueue_exception {};

    struct throwing_executor : public concurrencpp::executor {
        throwing_executor() : executor("throwing_executor") {}

        void enqueue(concurrencpp::task) override {
            throw executor_enqueue_exception();
        }

        void enqueue(std::span<concurrencpp::task>) override {
            throw executor_enqueue_exception();
        }

        int max_concurrency_level() const noexcept override {
            return 0;
        }

        bool shutdown_requested() const noexcept override {
            return false;
        }

        void shutdown() noexcept override {
            // do nothing
        }
    };
}  // namespace concurrencpp::tests

#endif  // CONCURRENCPP_RESULT_HELPERS_H
