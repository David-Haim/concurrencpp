#include "task.h"

using concurrencpp::task;
using concurrencpp::details::task_context;

task_context::task_context(task_context&& rhs) noexcept : m_pointer(nullptr) {
	*this = std::move(rhs);
}

task_context& task_context::operator = (task_context&& rhs) noexcept {
	if (static_cast<bool>(*this)) {
		clear();
	}

	if (!static_cast<bool>(rhs)) {
		return *this;
	}

	if (rhs.is_inlined()) {
		rhs.pointer()->move_to(*this);
		rhs.clear();
		return *this;
	}

	m_pointer = rhs.m_pointer;
	rhs.m_pointer = nullptr;
	return *this;
}

void task_context::clear() noexcept {
	const auto ptr = pointer();
	if (ptr == nullptr) {
		return;
	}

	if (is_inlined()) {
		ptr->~task_interface();
	}
	else {
		delete m_pointer;
	}

	m_pointer = nullptr;
}

const std::type_info& task_context::type() const noexcept {
	auto ptr = pointer();
	if (ptr == nullptr) {
		return typeid(std::nullptr_t);
	}

	return ptr->type();
}

/*
	task
*/

void task::clear() noexcept {
	m_ctx.clear();
}

void task::operator()() {
	if (!static_cast<bool>(m_ctx)) {
		return;
	}

	m_ctx.pointer()->execute();
}

void task::cancel(std::exception_ptr reason) noexcept {
	if (!static_cast<bool>(m_ctx)) {
		return;
	}

	m_ctx.pointer()->cancel(reason);
	clear();
}