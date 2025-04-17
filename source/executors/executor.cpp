#include "concurrencpp/executors/executor.h"
#include "concurrencpp/executors/constants.h"

#include "concurrencpp/errors.h"
#include "concurrencpp/threads/thread.h"

std::string concurrencpp::details::make_executor_worker_name(std::string_view executor_name) {
    return std::string(executor_name) + " worker";
}
