#ifndef CONCURRENCPP_EXECUTORS_H
#define CONCURRENCPP_EXECUTORS_H

#include "../forward_declerations.h"

#include "executor.h"
#include "../threads/thread_pool.h"
#include "../threads/thread_group.h"
#include "../threads/single_worker_thread.h"

#include <memory>
#include <thread>
#include <string_view>
#include <type_traits>

namespace concurrencpp {
	class inline_executor final : public executor {
		
		friend class ::concurrencpp::runtime;

		struct context {};

	public:		
		inline_executor(const inline_executor::context&) noexcept;

		virtual std::string_view name() const noexcept;	
		virtual void enqueue(task task);
	};

	class thread_pool_executor final : public executor {

		friend class ::concurrencpp::runtime;

		struct context {
			std::string_view cancellation_msg;
			size_t max_worker_count;
			std::chrono::seconds max_waiting_time;
		};

	private:
		details::thread_pool m_cpu_thread_pool;
	
	public:
		thread_pool_executor(const thread_pool_executor::context&);

		virtual std::string_view name() const noexcept;
		virtual void enqueue(task task);
	};

	class parallel_executor final : public executor {

		friend class ::concurrencpp::runtime;

		struct context {};

	private:
		std::shared_ptr<details::thread_pool> m_cpu_thread_pool;

	public:
		parallel_executor(
			std::shared_ptr<details::thread_pool> cpu_thread_pool,
			const parallel_executor::context&) noexcept;

		virtual std::string_view name() const noexcept;
		virtual void enqueue(task task);
	};

	class background_executor final : public executor {

		friend class ::concurrencpp::runtime;

		struct context {
			std::string_view cancellation_msg;
			size_t max_worker_count;
			std::chrono::seconds max_waiting_time;
		};


	private:
		details::thread_pool m_background_thread_pool;
	
	public:
		background_executor(const background_executor::context&);

		virtual std::string_view name() const noexcept;
		virtual void enqueue(task task);
	};

	class thread_executor final : public executor {

		friend class ::concurrencpp::runtime;

		struct context {};

	private:
		details::thread_group m_thread_group;

	public:
		thread_executor(const thread_executor::context&) noexcept;

		virtual std::string_view name() const noexcept;
		virtual void enqueue(task task);
	};

	class worker_thread_executor final : public executor {

		friend class ::concurrencpp::runtime;

	private:
		details::single_worker_thread m_worker;

		struct context {};

	public:	
		worker_thread_executor(const worker_thread_executor::context&);
	
		virtual std::string_view name() const noexcept;
		virtual void enqueue(task task);
	};

	class manual_executor final : public executor {

		friend class ::concurrencpp::runtime;

		struct context {};

	private:
		mutable std::mutex m_lock;
		details::array_deque<task> m_tasks;
		std::condition_variable m_condition;

	public:
		manual_executor(const manual_executor::context&);
		~manual_executor() noexcept;

		virtual std::string_view name() const noexcept;
		virtual void enqueue(task task);

		size_t size() const noexcept;
		bool empty() const noexcept;

		bool loop_once();
		size_t loop(size_t counts);

		void cancel_all(std::exception_ptr reason);
		
		void wait_for_task();
		bool wait_for_task(std::chrono::milliseconds max_waiting_time);
	};
}

#endif //CONCURRENCPP_EXECUTORS_H