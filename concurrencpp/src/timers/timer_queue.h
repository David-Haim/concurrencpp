#ifndef CONCURRENCPP_TIMER_QUEUE_H
#define CONCURRENCPP_TIMER_QUEUE_H

#include "timer.h"

#include "../utils/bind.h"
#include "../threads/thread.h"

#include <mutex>
#include <condition_variable>

#include <memory>
#include <chrono>
#include <vector>

#include <cassert>

namespace concurrencpp::details {
	enum class timer_request {
		add,
		remove
	};
}

namespace concurrencpp {
	class timer_queue : public std::enable_shared_from_this<timer_queue> {

	public:
		using timer_ptr = std::shared_ptr<details::timer_state_base>;
		using clock_type = std::chrono::high_resolution_clock;
		using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
		using request_queue = std::vector<std::pair<timer_ptr, details::timer_request>>;

		friend class concurrencpp::timer;

	private:
		std::mutex m_lock;
		request_queue m_request_queue;
		details::thread m_worker;
		std::condition_variable m_condition;
		bool m_abort;

		void ensure_worker_thread(std::unique_lock<std::mutex>& lock);

		void add_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer) noexcept;
		void remove_timer(timer_ptr existing_timer) noexcept;

		template<class callable_type>
		timer_ptr make_timer_impl(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			bool is_oneshot,
			callable_type&& callable) {

			assert(static_cast<bool>(executor));

			using decayed_type = typename std::decay_t<callable_type>;

			auto timer_core = std::make_shared<details::timer_state<decayed_type>>(
				due_time,
				frequency,
				std::move(executor),
				weak_from_this(),
				is_oneshot,
				std::forward<callable_type>(callable));

			std::unique_lock<std::mutex> lock(m_lock);
			ensure_worker_thread(lock);
			add_timer(lock, timer_core);
			return timer_core;
		}

		void work_loop() noexcept;

	public:
		timer_queue() noexcept;
		~timer_queue() noexcept;

		template<class callable_type>
		timer make_timer(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			callable_type&& callable) {

			if(!static_cast<bool>(executor)) {
				throw std::invalid_argument("concurrencpp::timer_queue::make_timer - executor is null.");
			}

			return make_timer_impl(
				due_time,
				frequency,
				std::move(executor),
				false,
				std::forward<callable_type>(callable));
		}

		template<class callable_type, class ... argumet_types>
		timer make_timer(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			callable_type&& callable,
			argumet_types&& ... arguments) {

			if (!static_cast<bool>(executor)) {
				throw std::invalid_argument("concurrencpp::timer_queue::make_timer - executor is null.");
			}

			return make_timer_impl(
				due_time,
				frequency,
				std::move(executor),
				false,
				details::bind(
					std::forward<callable_type>(callable),
					std::forward<argumet_types>(arguments)...));
		}

		template<class callable_type>
		timer make_one_shot_timer(
			size_t due_time,
			std::shared_ptr<concurrencpp::executor> executor,
			callable_type&& callable) {

			if (!static_cast<bool>(executor)) {
				throw std::invalid_argument("concurrencpp::timer_queue::make_one_shot_timer - executor is null.");
			}

			return make_timer_impl(
				due_time,
				0,
				std::move(executor),
				true,
				std::forward<callable_type>(callable));
		}

		template<class callable_type, class ... argumet_types>
		timer make_one_shot_timer(
			size_t due_time,
			std::shared_ptr<concurrencpp::executor> executor,
			callable_type&& callable,
			argumet_types&& ... arguments) {

			if (!static_cast<bool>(executor)) {
				throw std::invalid_argument("concurrencpp::timer_queue::make_one_shot_timer - executor is null.");
			}

			return make_timer_impl(
				due_time,
				0,
				std::move(executor),
				true,
				details::bind(
					std::forward<callable_type>(callable),
					std::forward<argumet_types>(arguments)...));
		}

		result<void> make_delay_object(size_t due_time, std::shared_ptr<concurrencpp::executor> executor);
	};
}

#endif //CONCURRENCPP_TIMER_QUEUE_H
