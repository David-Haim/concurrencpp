#ifndef CONCURRENCPP_EXECUTOR_ERROR_H
#define CONCURRENCPP_EXECUTOR_ERROR_H

#include "../forward_declerations.h"

#include <stdexcept>
#include <memory>

namespace concurrencpp::errors {
	struct executor_exception : public std::runtime_error {
		std::exception_ptr thrown_exeption;
		std::shared_ptr<concurrencpp::executor> throwing_executor;

		executor_exception(
			std::exception_ptr thrown_exeption,
			std::shared_ptr<concurrencpp::executor> throwing_executor) noexcept :
			runtime_error("concurrencpp::result - an exception was thrown while trying to enqueue result continuation."),
			thrown_exeption(thrown_exeption),
			throwing_executor(throwing_executor){}

		executor_exception(const executor_exception&) noexcept = default;
		executor_exception(executor_exception&&) noexcept = default;
	};
}

#endif