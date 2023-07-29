#include "concurrencpp/results/impl/result_state.h"
#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::result_state_base;

void result_state_base::assert_done() const noexcept {
    assert(m_pc_status.load(std::memory_order_acquire) == pc_status::producer_done);
}

void result_state_base::wait() noexcept {
    const auto status = m_pc_status.load(std::memory_order_acquire);
    if (status == pc_status::producer_done) {
        return;
    }

    auto expected_status = pc_status::idle;
    const auto idle = m_pc_status.compare_exchange_strong(expected_status,
                                                          pc_status::consumer_waiting,
                                                          std::memory_order_acq_rel,
                                                          std::memory_order_acquire);

    if (!idle) {
        assert_done();
        return;
    }

    atomic_wait(m_pc_status, pc_status::consumer_waiting, std::memory_order_acquire);
    assert_done();
}

result_state_base::pc_status result_state_base::wait_for(std::chrono::milliseconds ms) noexcept {
    const auto status = m_pc_status.load(std::memory_order_acquire);
    if (status == pc_status::producer_done) {
        return pc_status::producer_done;
    }

    auto expected_status = pc_status::idle;
    const auto idle = m_pc_status.compare_exchange_strong(expected_status,
                                                          pc_status::consumer_waiting,
                                                          std::memory_order_acq_rel,
                                                          std::memory_order_acquire);

    if (!idle) {
        assert_done();
        return pc_status::producer_done;
    }

    ms += std::chrono::milliseconds(2);
    const auto res = atomic_wait_for(m_pc_status, pc_status::consumer_waiting, ms, std::memory_order_acquire);
    if (res == atomic_wait_status::ok) {
        assert_done();
        return pc_status::producer_done;
    }

    assert(res == atomic_wait_status::timeout);

    // revert m_pc_state to idle
    expected_status = pc_status::consumer_waiting;
    const auto idle0 =
        m_pc_status.compare_exchange_strong(expected_status, pc_status::idle, std::memory_order_acq_rel, std::memory_order_acquire);

    if (idle0) {
        return pc_status::idle;
    }

    assert_done();
    return pc_status::producer_done;
}

bool result_state_base::await(coroutine_handle<void> caller_handle) noexcept {
    const auto status = m_pc_status.load(std::memory_order_acquire);
    if (status == pc_status::producer_done) {
        return false;  // don't suspend
    }

    m_consumer.set_await_handle(caller_handle);

    auto expected_status = pc_status::idle;
    const auto idle = m_pc_status.compare_exchange_strong(expected_status,
                                                          pc_status::consumer_set,
                                                          std::memory_order_acq_rel,
                                                          std::memory_order_acquire);

    if (!idle) {
        assert_done();
    }

    return idle;  // if idle = true, suspend
}

result_state_base::pc_status result_state_base::when_any(const std::shared_ptr<when_any_context>& when_any_state) noexcept {
    const auto status = m_pc_status.load(std::memory_order_acquire);
    if (status == pc_status::producer_done) {
        return status;
    }

    m_consumer.set_when_any_context(when_any_state);

    auto expected_status = pc_status::idle;
    const auto idle = m_pc_status.compare_exchange_strong(expected_status,
                                                          pc_status::consumer_set,
                                                          std::memory_order_acq_rel,
                                                          std::memory_order_acquire);

    if (!idle) {
        assert_done();
    }

    return status;
}

void result_state_base::share(const std::shared_ptr<shared_result_state_base>& shared_result_state) noexcept {
    const auto state = m_pc_status.load(std::memory_order_acquire);
    if (state == pc_status::producer_done) {
        return shared_result_state->on_result_finished();
    }

    m_consumer.set_shared_context(shared_result_state);

    auto expected_state = pc_status::idle;
    const auto idle = m_pc_status.compare_exchange_strong(expected_state,
                                                          pc_status::consumer_set,
                                                          std::memory_order_acq_rel,
                                                          std::memory_order_acquire);

    if (idle) {
        return;
    }

    assert_done();
    shared_result_state->on_result_finished();
}

void result_state_base::try_rewind_consumer() noexcept {
    const auto status = m_pc_status.load(std::memory_order_acquire);
    if (status != pc_status::consumer_set) {
        return;
    }

    auto expected_status = pc_status::consumer_set;
    const auto swapped =
        m_pc_status.compare_exchange_strong(expected_status, pc_status::idle, std::memory_order_acq_rel, std::memory_order_acquire);

    if (!swapped) {
        assert_done();
        return;
    }

    m_consumer.clear();
}
