#include "concurrencpp/results/impl/result_state.h"

using concurrencpp::details::await_context;
using concurrencpp::details::result_state_base;

void result_state_base::assert_done() const noexcept {
    assert(m_pc_state.load(std::memory_order_relaxed) == pc_state::producer);
}

void result_state_base::wait() {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer) {
        return;
    }

    auto wait_ctx = std::make_shared<wait_context>();
    m_consumer.set_wait_context(wait_ctx);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer, std::memory_order_acq_rel);

    if (!idle) {
        assert_done();
        return;
    }

    wait_ctx->wait();
    assert_done();
}

bool result_state_base::await(std::experimental::coroutine_handle<> caller_handle) noexcept {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer) {
        return false;  // don't suspend
    }

    m_consumer.set_await_context(caller_handle);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer, std::memory_order_acq_rel);

    if (!idle) {
        assert_done();
    }

    return idle;  // if idle = true, suspend
}

bool result_state_base::await_via(await_context& await_ctx, bool force_rescheduling) {
    auto handle_done_state = [this](await_context& await_ctx, bool force_rescheduling) -> bool {
        assert_done();

        if (!force_rescheduling) {
            return false;  // resume caller.
        }

        await_ctx(false);
        return true;
    };

    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer) {
        return handle_done_state(await_ctx, force_rescheduling);
    }

    m_consumer.set_await_via_context(&await_ctx);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer, std::memory_order_acq_rel);

    if (idle) {
        return true;
    }

    // the result is available
    return handle_done_state(await_ctx, force_rescheduling);
}

void result_state_base::when_all(std::shared_ptr<when_all_state_base> when_all_state) noexcept {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer) {
        return when_all_state->on_result_ready();
    }

    m_consumer.set_when_all_context(when_all_state);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer, std::memory_order_acq_rel);

    if (idle) {
        return;
    }

    assert_done();
    when_all_state->on_result_ready();
}

concurrencpp::details::when_any_status result_state_base::when_any(std::shared_ptr<when_any_state_base> when_any_state, size_t index) noexcept {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer) {
        return when_any_status::result_ready;
    }

    m_consumer.set_when_any_context(std::move(when_any_state), index);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer, std::memory_order_acq_rel);

    if (idle) {
        return when_any_status::set;
    }

    // no need to continue the iteration of when_any, we've found a ready task.
    // tell all predecessors to rewind their state.
    assert_done();
    return when_any_status::result_ready;
}

void result_state_base::try_rewind_consumer() noexcept {
    const auto pc_state = this->m_pc_state.load(std::memory_order_acquire);
    if (pc_state != pc_state::consumer) {
        return;
    }

    auto expected_consumer_state = pc_state::consumer;
    const auto consumer = this->m_pc_state.compare_exchange_strong(expected_consumer_state, pc_state::idle, std::memory_order_acq_rel);

    if (!consumer) {
        assert_done();
        return;
    }

    m_consumer.clear();
}

void result_state_base::publish_result() noexcept {
    const auto state_before = this->m_pc_state.exchange(pc_state::producer, std::memory_order_acq_rel);

    assert(state_before != pc_state::producer);

    if (state_before == pc_state::idle) {
        return;
    }

    assert(state_before == pc_state::consumer);
    m_consumer();
}
