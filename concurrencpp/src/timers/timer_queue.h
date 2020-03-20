#ifndef CONCURRENCPP_TIMER_QUEUE_H
#define CONCURRENCPP_TIMER_QUEUE_H

#include "timer.h"
#include "../utils/bind.h"

#include <mutex>
#include <thread>
#include <condition_variable>

#include <memory>
#include <chrono>

#include <cassert>

namespace concurrencpp {
	class timer_queue : public std::enable_shared_from_this<timer_queue>{

		using clock_type = std::chrono::high_resolution_clock;
		using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
		using timer_ptr = std::shared_ptr<concurrencpp::details::timer_impl_base>;

		friend class concurrencpp::timer;

		enum class status {
			idle,
			running,
			new_timer,
			cancelled
		};

	private:
		std::mutex m_lock;
		status m_status;
		std::condition_variable m_condition;
		std::shared_ptr<details::timer_impl_base> m_timers;
		std::thread m_worker;

		static void add_timer(timer_ptr& head, timer_ptr new_timer) noexcept;

		void ensure_worker_thread(std::unique_lock<std::mutex>& lock);
		void register_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer);
		
		void remove_timer_impl(timer_ptr existing_timer);
		void remove_timer(timer_ptr existing_timer);
	
		time_point process_timers();

		template<class given_functor_type>
		timer_ptr create_timer_impl(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			given_functor_type&& functor) {

			using decayed_type = typename std::decay_t<given_functor_type>;
			auto timer_core = std::make_shared<details::timer_impl<decayed_type>>(
				due_time,
				frequency,
				std::move(executor),
				weak_from_this(),
				std::forward<given_functor_type>(functor));
		
			std::unique_lock<std::mutex> lock(m_lock);
			ensure_worker_thread(lock);
			register_timer(lock, timer_core);
			return timer_core;
		}
	
		void work_loop() noexcept;

	public:
		timer_queue() noexcept;
		~timer_queue() noexcept;

		template<class functor_type>
		timer create_timer(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			functor_type&& functor) {
			return create_timer_impl(
				due_time,
				frequency,
				std::move(executor),
				std::forward<functor_type>(functor));
		}

		template<class functor_type, class ... argumet_types>
		timer create_timer(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			functor_type&& functor,
			argumet_types&& ... arguments) {
			return create_timer_impl(
				due_time,
				frequency,
				std::move(executor),
				details::bind(
					std::forward<functor_type>(functor),
					std::forward<argumet_types>(arguments)...));
		}

		template<class functor_type>
		timer create_one_shot_timer(
			size_t due_time,
			std::shared_ptr<concurrencpp::executor> executor,
			functor_type&& functor) {
			return create_timer_impl(
				due_time,
				details::timer_traits::k_oneshot_timer_frequency,
				std::move(executor),
				std::forward<functor_type>(functor));
		}

		template<class functor_type, class ... argumet_types>
		timer create_one_shot_timer(
			size_t due_time,
			std::shared_ptr<concurrencpp::executor> executor,
			functor_type&& functor,
			argumet_types&& ... arguments) {
			return create_timer_impl(
				due_time,
				details::timer_traits::k_oneshot_timer_frequency,
				std::move(executor),
				details::bind(
					std::forward<functor_type>(functor),
					std::forward<argumet_types>(arguments)...));
		}

		result<void> create_delay_object(size_t due_time, std::shared_ptr<concurrencpp::executor> executor);
	};
}

#endif //CONCURRENCPP_TIMER_QUEUE_H
