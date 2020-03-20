#ifndef CONCURRENCPP_RUNTIME_H
#define CONCURRENCPP_RUNTIME_H

#include "../forward_declerations.h"

#include "../timers/timer_queue.h"
#include "../executors/executors.h"

namespace concurrencpp::details {
	struct thread_interrupter {};

	void interrupt_thread();
}

namespace concurrencpp {
	struct runtime_options {
		size_t max_cpu_threads;
		std::chrono::seconds max_cpu_thread_waiting_time;

		size_t max_background_threads;
		std::chrono::seconds max_background_thread_waiting_time;
	
		runtime_options() noexcept;

		runtime_options(const runtime_options&) noexcept = default;
		runtime_options& operator = (const runtime_options&) noexcept = default;
	};

	class runtime : public std::enable_shared_from_this<runtime> {		

		friend std::shared_ptr<runtime> make_runtime();
		friend std::shared_ptr<runtime> make_runtime(const runtime_options&);

	private:
		std::shared_ptr<timer_queue> m_timer_queue;
	
		std::shared_ptr<inline_executor> m_inline_executor; 
		std::shared_ptr<thread_pool_executor> m_thread_pool_executor;
		std::shared_ptr<background_executor> m_background_executor;
		std::shared_ptr<thread_executor> m_thread_executor;

		struct context{};

	public:
		runtime(const context& ctx);
		runtime(const context& ctx, const runtime_options& options);

		~runtime() noexcept;	

		std::shared_ptr<timer_queue> timer_queue() const noexcept;

		std::shared_ptr<inline_executor> inline_executor() noexcept;
		std::shared_ptr<thread_pool_executor> thread_pool_executor()  noexcept;
		std::shared_ptr<background_executor> background_executor()  noexcept;
		std::shared_ptr<thread_executor> thread_executor()  noexcept;
		std::shared_ptr<worker_thread_executor> worker_thread_executor() const;
		std::shared_ptr<manual_executor> manual_executor() const;

		static std::tuple<unsigned int, unsigned int, unsigned int> version() noexcept;
	};

	std::shared_ptr<runtime> make_runtime();
	std::shared_ptr<runtime> make_runtime(const runtime_options&);
}

#endif