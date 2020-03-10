#include "timer.h"
#include "timer_queue.h"

#include "../errors.h"
#include "../result/result.h"
#include "../executors/executor.h"

using concurrencpp::timer;
using concurrencpp::details::timer_impl;
using concurrencpp::details::timer_action;
using concurrencpp::details::timer_impl_base;

timer_impl_base::timer_impl_base(
	size_t due_time,
	size_t frequency,
	std::shared_ptr<concurrencpp::executor> executor,
	std::weak_ptr<concurrencpp::timer_queue> timer_queue) noexcept :
	m_timer_queue(std::move(timer_queue)),
	m_executor(std::move(executor)),
	m_due_time(due_time),
	m_frequency(frequency),
	m_deadline(make_deadline(milliseconds(due_time))),
	m_prev(nullptr){
	assert(static_cast<bool>(m_executor));
}

timer_action timer_impl_base::update(const time_point now) { //returns true if timer should be re-scheduled. false if cancelled/oneshote
	const auto deadline = m_deadline;	
	if (deadline > now) {
		return timer_action::idle; //idle, deadline hasn't reached
	}

	assert(!m_timer_queue.expired());

	const auto frequency = m_frequency.load(std::memory_order_acquire);
	if (frequency == timer_traits::k_oneshot_timer_frequency) {
		return timer_action::fire_delete; //oneshot timer. shouldn't be re-enqueued
	}
	
	m_deadline = make_deadline(milliseconds(frequency));
	return timer_action::fire;
}

void timer_impl_base::schedule() {
	assert(!m_timer_queue.expired());
	assert(static_cast<bool>(m_executor));

	m_executor->enqueue([self = shared_from_this()]() mutable {
		self->execute();
	});
}

std::chrono::time_point<std::chrono::high_resolution_clock> timer_impl_base::get_deadline() const noexcept {
	return m_deadline;
}

size_t timer_impl_base::get_frequency() const noexcept {
	return m_frequency.load(std::memory_order_acquire);
}

size_t timer_impl_base::get_due_time() const noexcept {
	return m_due_time; //no need to synchronize, const anyway.
}

std::shared_ptr<concurrencpp::executor> timer_impl_base::get_executor() const noexcept {
	return m_executor;
}

std::weak_ptr<concurrencpp::timer_queue> timer_impl_base::get_timer_queue() const noexcept {
	return m_timer_queue;
}

void timer_impl_base::set_next(std::shared_ptr<timer_impl_base> next) noexcept {
	m_next = std::move(next);
}

std::shared_ptr<timer_impl_base> timer_impl_base::get_next(bool nullify_next) noexcept {
	if (nullify_next) {
		return std::move(m_next);
	}

	return m_next;
}

void timer_impl_base::set_prev(timer_impl_base* prev) noexcept {
	m_prev = prev;
}

timer_impl_base* timer_impl_base::get_prev(bool nullify_prev) noexcept {
	auto prev = m_prev;

	if (nullify_prev) {
		m_prev = nullptr;
	}

	return prev;
}

void timer_impl_base::set_new_frequency(size_t new_frequency) noexcept {
	m_frequency.store(new_frequency, std::memory_order_release);
}

timer::timer(std::shared_ptr<timer_impl_base> timer_impl) noexcept : m_impl(std::move(timer_impl)){}

timer::~timer() noexcept {
	cancel();
}

void timer::throw_if_empty(const char* error_message) const {
	if (!static_cast<bool>(m_impl)) {
		throw concurrencpp::errors::empty_timer(error_message);
	}
}

size_t timer::get_due_time() const {
	throw_if_empty("timer::get_due_time() - timer is empty.");
	return m_impl->get_due_time();
}

size_t timer::get_frequency() const {
	throw_if_empty("timer::get_frequency() - timer is empty.");
	return m_impl->get_frequency();
}

std::shared_ptr<concurrencpp::executor> timer::get_executor() const {
	throw_if_empty("timer::get_executor() - timer is empty.");
	return m_impl->get_executor();
}

std::weak_ptr<concurrencpp::timer_queue> timer::get_timer_queue() const {
	throw_if_empty("timer::get_timer_queue() - timer is empty.");
	return m_impl->get_timer_queue();
}

void timer::cancel() {
	if (!static_cast<bool>(m_impl)) {
		return;
	}

	auto impl = std::move(m_impl);
	auto timer_queue = impl->get_timer_queue().lock();
	
	if (!static_cast<bool>(timer_queue)) {		
		return;
	}

	timer_queue->remove_timer(impl);
}

void timer::set_frequency(size_t new_frequency) {
	throw_if_empty("timer::get_timer_queue() - timer is empty.");
	return m_impl->set_new_frequency(new_frequency);
}

timer& timer::operator = (timer&& rhs) noexcept {
	if (this == &rhs) {
		return *this;
	}

	if (static_cast<bool>(*this)) {
		cancel();
	}

	m_impl.reset();
	m_impl = std::move(rhs.m_impl);
	return *this;
}
