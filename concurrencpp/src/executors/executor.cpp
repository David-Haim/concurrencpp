#include "executor.h"
#include "constants.h"

#include "../errors.h"
#include "../threads/thread.h"

void concurrencpp::details::throw_executor_shutdown_exception(std::string_view executor_name) {
	const auto error_msg = std::string(executor_name) + consts::k_executor_shutdown_err_msg;
	throw errors::executor_shutdown(error_msg);
}

std::string concurrencpp::details::make_executor_worker_name(std::string_view executor_name) {
	return std::string(executor_name) + " worker";
}
