#include "result_core.h"
#include "../executors/executor.h"

using concurrencpp::details::wait_context;
using concurrencpp::details::await_context;
using concurrencpp::details::result_core_base;
using concurrencpp::details::result_core_per_thread_data;

thread_local result_core_per_thread_data result_core_per_thread_data::s_tl_per_thread_data;

void wait_context::wait() noexcept {
	std::unique_lock<std::mutex> lock(m_lock);
	m_condition.wait(lock, [this] {
		return m_ready;
	});
}

bool wait_context::wait_for(size_t milliseconds) noexcept {
	std::unique_lock<std::mutex> lock(m_lock);
	return m_condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] {
		return m_ready;
	});
}

void wait_context::notify() noexcept {
	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_ready = true;
	}

	m_condition.notify_all();
}

std::shared_ptr<wait_context> wait_context::make() {
	return std::make_shared<wait_context>();
}

void result_core_base::wait() {
	const auto state = m_pc_state.load(std::memory_order_acquire);
	if (state == pc_state::producer) {
		return;
	}

	auto wait_ctx = wait_context::make();

	assert_consumer_idle();
	m_consumer.template emplace<3>(wait_ctx);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (!idle) {
		assert_done();
		clear_consumer();
		return;
	}

	wait_ctx->wait();
	assert_done();
}

bool result_core_base::await(std::experimental::coroutine_handle<> caller_handle) noexcept {
	assert_consumer_idle();
	m_consumer.template emplace<1>(caller_handle);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (!idle) {
		assert_done();
		clear_consumer();
	}

	return idle; //if idle = true, suspend
}

bool result_core_base::await_via(
	std::shared_ptr<concurrencpp::executor> executor,
	std::experimental::coroutine_handle<> caller_handle,
	bool force_rescheduling) {
	assert(static_cast<bool>(executor));

	assert_consumer_idle();
	m_consumer.template emplace<2>(std::move(executor), caller_handle);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (idle) {
		return true;
	}

	//the result is available
	assert_done();

	if (!force_rescheduling) {
		clear_consumer();
		return false; //resume caller.
	}

	try {
		schedule_coroutine(std::get<2>(m_consumer));
	}
	catch (...) {
		clear_consumer();
		throw;
	}

	return true;
}

void result_core_base::schedule_coroutine(
	concurrencpp::executor& executor,
	std::experimental::coroutine_handle<> coro_handle) {
	assert(static_cast<bool>(coro_handle));
	assert(!coro_handle.done());
	executor.enqueue(coro_handle);
}

void result_core_base::schedule_coroutine(await_context& await_ctx) {
	auto executor = await_ctx.first.get();
	auto coro_handle = await_ctx.second;

	assert(executor != nullptr);
	schedule_coroutine(*executor, coro_handle);
}

bool result_core_base::initial_reschedule(std::experimental::coroutine_handle<> handle) {
	auto executor_ptr = std::exchange(result_core_per_thread_data::s_tl_per_thread_data.executor, nullptr);
	if (executor_ptr != nullptr) {
		executor_ptr->enqueue(handle);
		return true;
	}

	auto accumulator_ptr = std::exchange(result_core_per_thread_data::s_tl_per_thread_data.accumulator, nullptr);
	if (accumulator_ptr != nullptr) {
		accumulator_ptr->push_back(handle);
		return true;
	}

	return false; //no executor, resume inline (don't suspend)
}