#include "timer_queue.h"
#include "timer.h"

#include "../errors.h"
#include "../result/result.h"

#include <cassert>

using namespace std::chrono;
using concurrencpp::timer;
using concurrencpp::timer_queue;
using concurrencpp::details::timer_impl_base;

timer_queue::timer_queue() noexcept : m_status(status::idle){}

timer_queue::~timer_queue() noexcept {	
	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		if (m_status == status::idle) {
			assert(!m_worker.joinable());
			return;
		}

		m_status = status::cancelled;
	}

	m_condition.notify_all();
	assert(m_worker.joinable());
	m_worker.join();
}

void timer_queue::add_timer(timer_ptr& head, timer_ptr new_timer) noexcept {
	if (!static_cast<bool>(head)) {
		head = std::move(new_timer);
		return;
	}
	
	auto new_timer_ptr = new_timer.get();
	auto head_ptr = head.get();

	auto prev_head = std::move(head);
	head = std::move(new_timer);
	head->set_next(std::move(prev_head));

	if (head_ptr != nullptr) {
		head_ptr->set_prev(new_timer_ptr);
	}
}

void timer_queue::register_timer(std::unique_lock<std::mutex>& lock, std::shared_ptr<timer_impl_base> new_timer) {
	assert(lock.owns_lock());

	add_timer(m_timers, std::move(new_timer));
	m_status = status::new_timer;

	lock.unlock();
	m_condition.notify_all();
}

void timer_queue::remove_timer_impl(std::shared_ptr<timer_impl_base> existing_timer) {
	//NOTE: NOT THREAD SAFE.

	if (existing_timer == m_timers) {
		auto next = m_timers->get_next(true);
		m_timers.reset();
		m_timers = std::move(next);
		return;
	}

	auto prev_timer = existing_timer->get_prev(true);
	auto next_timer = existing_timer->get_next(true);
	auto next_timer_ptr = next_timer.get();

	if (prev_timer != nullptr) {
		prev_timer->set_next(std::move(next_timer));
	}

	if (static_cast<bool>(next_timer)) {
		next_timer->set_prev(prev_timer);
	}
}

void timer_queue::remove_timer(std::shared_ptr<timer_impl_base> existing_timer) {
	std::unique_lock<decltype(m_lock)> lock(m_lock);
	remove_timer_impl(std::move(existing_timer));
}

time_point<std::chrono::high_resolution_clock> timer_queue::process_timers() {
	auto cursor = m_timers;
	const auto now = std::chrono::high_resolution_clock::now();
	auto nearest_time_point = now + std::chrono::hours(24 * 14);

	while (static_cast<bool>(cursor)) {
		auto timer = cursor;
		cursor = cursor->get_next();

		const auto action = timer->update(now);		
		if (action == details::timer_action::fire) {
			timer->schedule();
		}
		else if (action == details::timer_action::fire_delete) {
			timer->schedule();
			remove_timer_impl(timer);
		}

		const auto deadline = timer->get_deadline();
		if (deadline < nearest_time_point) {
			nearest_time_point = deadline;
		}
	}

	return nearest_time_point;
}

void timer_queue::work_loop() noexcept {
	{
		std::unique_lock<decltype(m_lock)> lock(m_lock);
		assert(m_status != status::idle);
	}

	try {
		while (true) {
			std::unique_lock<decltype(m_lock)> lock(m_lock);
			if (m_status == status::cancelled) {
				return;
			}

			const auto deadline = process_timers();

			if (m_status == status::cancelled) {
				return;
			}

			const auto now = clock_type::now();
			if (deadline <= now) {
				continue;
			}	

			m_condition.wait_until(lock, deadline, [this] {
				return (m_status == status::cancelled) || (m_status == status::new_timer);
			});

			if (m_status == status::cancelled) {
				return;
			}
	
			if (m_status == status::new_timer) {
				m_status = status::running;
			}
		}
	}
	catch (...) {
		std::abort();
	}
}

void timer_queue::ensure_worker_thread(std::unique_lock<std::mutex>& lock) {
	assert(lock.owns_lock());
	
	if (m_status == status::idle) {
		assert(!m_worker.joinable());
	
		m_worker = std::thread([this] {
			work_loop();
		});

		m_status = status::running;
		return;
	}

	assert(m_worker.joinable());
}

concurrencpp::result<void> timer_queue::create_delay_object(size_t due_time, std::shared_ptr<concurrencpp::executor> executor) {
	concurrencpp::result_promise<void> promise;
	auto task = promise.get_result();

	auto ignored_state = create_timer_impl(
		due_time,
		details::timer_traits::k_oneshot_timer_frequency,
		std::move(executor),
		[tcs = std::move(promise)]() mutable { tcs.set_result(); });

	return task;
}