#ifndef CONCURRENCPP_EXECUTOR_TEST_HELPERS_H
#define CONCURRENCPP_EXECUTOR_TEST_HELPERS_H

#include "concurrencpp/executors/executor.h"

namespace concurrencpp::tests {
    struct executor_shutdowner {
        std::shared_ptr<concurrencpp::executor> executor;

        executor_shutdowner(std::shared_ptr<concurrencpp::executor> executor) noexcept : executor(std::move(executor)) {}

        ~executor_shutdowner() noexcept {
            executor->shutdown();
        }
    };
}  // namespace concurrencpp::tests

#endif
