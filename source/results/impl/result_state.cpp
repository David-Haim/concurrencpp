#include "concurrencpp/results/impl/result_state.h"
#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::result_state_base;

void result_state_base::assert_done() const noexcept {
    assert(m_pc_state.load(std::memory_order_acquire) == pc_state::producer_done);
}

void result_state_base::wait() {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer_done) {
        return;
    }

    const auto wait_ctx = std::make_shared<wait_context>();
    m_consumer.set_wait_context(wait_ctx);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer_set, std::memory_order_acq_rel);

    if (!idle) {
        assert_done();
        return;
    }

    wait_ctx->wait();
    assert_done();
}

bool result_state_base::await(coroutine_handle<void> caller_handle) noexcept {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer_done) {
        return false;  // don't suspend
    }

    m_consumer.set_await_handle(caller_handle);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer_set, std::memory_order_acq_rel);

    if (!idle) {
        assert_done();
    }

    return idle;  // if idle = true, suspend
}

result_state_base::pc_state result_state_base::when_any(const std::shared_ptr<when_any_context>& when_any_state) noexcept {
    const auto state = m_pc_state.load(std::memory_order_acquire);
    if (state == pc_state::producer_done) {
        return state;
    }

    m_consumer.set_when_any_context(when_any_state);

    auto expected_state = pc_state::idle;
    const auto idle = m_pc_state.compare_exchange_strong(expected_state, pc_state::consumer_set, std::memory_order_acq_rel);

    if (!idle) {
        assert_done();
    }

    return state;
}

void result_state_base::try_rewind_consumer() noexcept {
    const auto pc_state = m_pc_state.load(std::memory_order_acquire);
    if (pc_state != pc_state::consumer_set) {
        return;
    }

    auto expected_consumer_state = pc_state::consumer_set;
    const auto consumer = m_pc_state.compare_exchange_strong(expected_consumer_state, pc_state::idle, std::memory_order_acq_rel);

    if (!consumer) {
        assert_done();
        return;
    }

    m_consumer.clear();
}