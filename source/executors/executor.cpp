#include "concurrencpp/errors.h"
#include "concurrencpp/executors/executor.h"
#include "concurrencpp/executors/constants.h"

void concurrencpp::executor::throw_runtime_shutdown_exception() {
    const auto error_msg = name + details::consts::k_executor_shutdown_err_msg;
    throw errors::runtime_shutdown(error_msg);
}

std::string concurrencpp::executor::make_executor_worker_name() {
    return name + " worker";
}
