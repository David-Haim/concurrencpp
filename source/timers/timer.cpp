#include "concurrencpp/timers/timer.h"
#include "concurrencpp/timers/timer_queue.h"
#include "concurrencpp/timers/constants.h"

#include "concurrencpp/errors.h"
#include "concurrencpp/results/result.h"
#include "concurrencpp/executors/executor.h"

using concurrencpp::timer;
using concurrencpp::details::timer_state;
using concurrencpp::details::timer_state_base;

timer_state_base::timer_state_base(size_t due_time,
                                   size_t frequency,
                                   std::shared_ptr<concurrencpp::executor> executor,
                                   std::weak_ptr<concurrencpp::timer_queue> timer_queue,
                                   bool is_oneshot) noexcept :
    m_timer_queue(std::move(timer_queue)),
    m_executor(std::move(executor)), m_due_time(due_time), m_frequency(frequency), m_deadline(make_deadline(milliseconds(due_time))),
    m_cancelled(false), m_is_oneshot(is_oneshot) {
    assert(static_cast<bool>(m_executor));
}

void timer_state_base::fire() {
    const auto frequency = m_frequency.load(std::memory_order_relaxed);
    m_deadline = make_deadline(milliseconds(frequency));

    assert(static_cast<bool>(m_executor));

    m_executor->post([self = shared_from_this()]() mutable {
        self->execute();
    });
}

timer::timer(std::shared_ptr<timer_state_base> timer_impl) noexcept : m_state(std::move(timer_impl)) {}

timer::~timer() noexcept {
    cancel();
}

void timer::throw_if_empty(const char* error_message) const {
    if (static_cast<bool>(m_state)) {
        return;
    }

    throw errors::empty_timer(error_message);
}

std::chrono::milliseconds timer::get_due_time() const {
    throw_if_empty(details::consts::k_timer_empty_get_due_time_err_msg);
    return std::chrono::milliseconds(m_state->get_due_time());
}

std::chrono::milliseconds timer::get_frequency() const {
    throw_if_empty(details::consts::k_timer_empty_get_frequency_err_msg);
    return std::chrono::milliseconds(m_state->get_frequency());
}

std::shared_ptr<concurrencpp::executor> timer::get_executor() const {
    throw_if_empty(details::consts::k_timer_empty_get_executor_err_msg);
    return m_state->get_executor();
}

std::weak_ptr<concurrencpp::timer_queue> timer::get_timer_queue() const {
    throw_if_empty(details::consts::k_timer_empty_get_timer_queue_err_msg);
    return m_state->get_timer_queue();
}

void timer::cancel() {
    if (!static_cast<bool>(m_state)) {
        return;
    }

    auto state = std::move(m_state);
    state->cancel();

    auto timer_queue = state->get_timer_queue().lock();

    if (!static_cast<bool>(timer_queue)) {
        return;
    }

    timer_queue->remove_internal_timer(std::move(state));
}

void timer::set_frequency(std::chrono::milliseconds new_frequency) {
    throw_if_empty(details::consts::k_timer_empty_set_frequency_err_msg);
    return m_state->set_new_frequency(new_frequency.count());
}

timer& timer::operator=(timer&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    if (static_cast<bool>(*this)) {
        cancel();
    }

    m_state = std::move(rhs.m_state);
    return *this;
}
