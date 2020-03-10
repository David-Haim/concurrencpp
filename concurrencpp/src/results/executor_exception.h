#ifndef CONCURRENCPP_EXECUTOR_ERROR_H
#define CONCURRENCPP_EXECUTOR_ERROR_H

#include "constants.h"

#include "../forward_declerations.h"

#include <stdexcept>
#include <memory>

namespace concurrencpp::errors {
	struct executor_exception final : public std::runtime_error {
		std::exception_ptr thrown_exception;
		std::shared_ptr<concurrencpp::executor> throwing_executor;

		executor_exception(
			std::exception_ptr thrown_exception,
			std::shared_ptr<executor> throwing_executor) noexcept :
			runtime_error(details::consts::k_executor_exception_error_msg),
			thrown_exception(thrown_exception),
			throwing_executor(throwing_executor) {}

		executor_exception(const executor_exception&) noexcept = default;
		executor_exception(executor_exception&&) noexcept = default;
	};
}

#endif