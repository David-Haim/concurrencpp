#include "result_core.h"
#include "../executors/executor.h"

using concurrencpp::details::wait_context;
using concurrencpp::details::await_context;
using concurrencpp::details::result_core_base;

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

void result_core_base::wait() {
	const auto state = m_pc_state.load(std::memory_order_acquire);
	if (state == pc_state::producer) {
		return;
	}

	auto wait_ctx = std::make_shared<wait_context>();

	assert_consumer_idle();
	m_consumer.emplace<3>(wait_ctx);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (!idle) {
		assert_done();
		return;
	}

	wait_ctx->wait();
	assert_done();
}

bool result_core_base::await(std::experimental::coroutine_handle<> caller_handle) noexcept {
	const auto state = m_pc_state.load(std::memory_order_acquire);
	if (state == pc_state::producer) {
		return false; //don't suspend
	}

	assert_consumer_idle();
	m_consumer.emplace<1>(caller_handle);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (!idle) {
		assert_done();
	}

	return idle; //if idle = true, suspend
}

bool result_core_base::await_via(
	std::shared_ptr<concurrencpp::executor> executor,
	std::experimental::coroutine_handle<> caller_handle,
	bool force_rescheduling) {
	assert(static_cast<bool>(executor));
	auto handle_done_state = [this] (await_context& await_ctx, bool force_rescheduling) -> bool {
		assert_done();

		if (!force_rescheduling) {
			return false; //resume caller.
		}

		await_ctx.first->enqueue(await_ctx.second);
		return true;
	};

	const auto state = m_pc_state.load(std::memory_order_acquire);
	if (state == pc_state::producer) {
		await_context await_ctx(std::move(executor), caller_handle);
		return handle_done_state(await_ctx, force_rescheduling);
	}

	assert_consumer_idle();
	m_consumer.emplace<2>(std::move(executor), caller_handle);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (idle) {
		return true;
	}

	//the result is available
	auto await_ctx = std::move(std::get<2>(m_consumer));
	return handle_done_state(await_ctx, force_rescheduling);
}

void result_core_base::when_all(std::shared_ptr<when_all_state_base> when_all_state) noexcept {
	const auto state = m_pc_state.load(std::memory_order_acquire);
	if (state == pc_state::producer) {
		return when_all_state->on_result_ready();
	}

	assert_consumer_idle();
	m_consumer.emplace<4>(std::move(when_all_state));

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (idle) {
		return;
	}

	assert_done();
	auto& state_ptr = std::get<4>(m_consumer);
	state_ptr->on_result_ready();
}

concurrencpp::details::when_any_status result_core_base::when_any(
	std::shared_ptr<when_any_state_base> when_any_state,
	size_t index) noexcept {
	const auto state = m_pc_state.load(std::memory_order_acquire);
	if (state == pc_state::producer) {
		return when_any_status::result_ready;
	}

	assert_consumer_idle();
	m_consumer.emplace<5>(std::move(when_any_state), index);

	auto expected_state = pc_state::idle;
	const auto idle = m_pc_state.compare_exchange_strong(
		expected_state,
		pc_state::consumer,
		std::memory_order_acq_rel);

	if (idle) {
		return when_any_status::set;
	}

	//no need to continue the iteration of when_any, we've found a ready task.
	//tell all predecessors to rewind their state.
	assert_done();
	return when_any_status::result_ready;
}

void result_core_base::try_rewind_consumer() noexcept {
	const auto pc_state = this->m_pc_state.load(std::memory_order_acquire);
	if (pc_state != pc_state::consumer) {
		return;
	}

	auto expected_consumer_state = pc_state::consumer;
	const auto consumer = this->m_pc_state.compare_exchange_strong(
		expected_consumer_state,
		pc_state::idle,
		std::memory_order_acq_rel);

	if (!consumer) {
		assert_done();
		return;
	}

	m_consumer.emplace<0>();
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
