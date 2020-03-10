#ifndef CONCURRENCPP_INLINE_EXECUTOR_H
#define CONCURRENCPP_INLINE_EXECUTOR_H

#include "executor.h"
#include "constants.h"

namespace concurrencpp {
	class inline_executor final : public executor {

	private:
		std::atomic_bool m_abort;

		void throw_if_aborted() const {
			if (!m_abort.load(std::memory_order_relaxed)) {
				return;
			}

			details::throw_executor_shutdown_exception(name);
		}

	public:
		inline_executor() noexcept :
			executor(details::consts::k_inline_executor_name),
			m_abort(false) {}

		void enqueue(std::experimental::coroutine_handle<> task) override {
			throw_if_aborted();
			task();
		}

		void enqueue(std::span<std::experimental::coroutine_handle<>> tasks) override {
			throw_if_aborted();

			for (auto& task : tasks) {
				task();
			}
		}

		int max_concurrency_level() const noexcept override {
			return details::consts::k_inline_executor_max_concurrency_level;
		}

		void shutdown() noexcept override {
			m_abort.store(true, std::memory_order_relaxed);
		}

		bool shutdown_requested() const noexcept override {
			return m_abort.load(std::memory_order_relaxed);
		}
	};
}

#endif