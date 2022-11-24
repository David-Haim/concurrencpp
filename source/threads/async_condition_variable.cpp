#include "concurrencpp/threads/constants.h"
#include "concurrencpp/threads/async_condition_variable.h"

using concurrencpp::executor;
using concurrencpp::lazy_result;
using concurrencpp::scoped_async_lock;
using concurrencpp::async_condition_variable;

/*
    async_condition_variable::cv_awaitable
*/

async_condition_variable::cv_awaitable::cv_awaitable(async_condition_variable& parent,
                                                     scoped_async_lock& lock,
                                                     const std::shared_ptr<executor>& resume_executor) noexcept :
    m_parent(parent),
    m_lock(lock), m_resume_executor(resume_executor) {}

void async_condition_variable::cv_awaitable::resume() noexcept {
    assert(static_cast<bool>(m_caller_handle));
    assert(!m_caller_handle.done());
    m_resume_executor->post(details::await_via_functor {m_caller_handle, &m_interrupted});
}

void async_condition_variable::cv_awaitable::await_suspend(details::coroutine_handle<void> caller_handle) {
    m_caller_handle = caller_handle;

    std::unique_lock<std::mutex> lock(m_parent.m_lock);
    m_lock.unlock();

    m_parent.m_awaiters.push_back(this);
}

void async_condition_variable::cv_awaitable::await_resume() const {
    if (m_interrupted) {
        throw errors::broken_task(details::consts::k_broken_task_exception_error_msg);
    }
}

/*
    async_condition_variable::slist
*/

async_condition_variable::slist::slist(slist&& rhs) noexcept :
    m_head(std::exchange(rhs.m_head, nullptr)), m_tail(std::exchange(rhs.m_tail, nullptr)) {}

bool async_condition_variable::slist::empty() const noexcept {
    return m_head == nullptr;
}

void async_condition_variable::slist::push_back(cv_awaitable* node) noexcept {
    assert(node->next == nullptr);

    if (m_head == nullptr) {
        assert(m_tail == nullptr);
        m_head = node;
        m_tail = node;
        return;
    }

    assert(m_tail != nullptr);
    m_tail->next = node;
    m_tail = node;
}

async_condition_variable::cv_awaitable* async_condition_variable::slist::pop_front() noexcept {
    if (m_head == nullptr) {
        assert(m_tail == nullptr);
        return nullptr;
    }

    assert(m_tail != nullptr);
    const auto node = m_head;
    m_head = m_head->next;

    if (m_head == nullptr) {
        m_tail = nullptr;
    }

    return node;
}

/*
    async_condition_variable
*/

async_condition_variable::~async_condition_variable() noexcept {
#ifdef CRCPP_DEBUG_MODE
    std::unique_lock<std::mutex> lock(m_lock);
    assert(m_awaiters.empty() && "concurrencpp::async_condition_variable is deleted while being used.");
#endif
}

void async_condition_variable::verify_await_params(const std::shared_ptr<executor>& resume_executor, const scoped_async_lock& lock) {
    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument(details::consts::k_async_condition_variable_await_invalid_resume_executor_err_msg);
    }

    if (!lock.owns_lock()) {
        throw std::invalid_argument(details::consts::k_async_condition_variable_await_lock_unlocked_err_msg);
    }
}

lazy_result<void> async_condition_variable::await_impl(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock) {
    co_await cv_awaitable(*this, lock, resume_executor);

    assert(!lock.owns_lock());
    co_await lock.lock(resume_executor);
}

lazy_result<void> async_condition_variable::await(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock) {
    verify_await_params(resume_executor, lock);
    return await_impl(std::move(resume_executor), lock);
}

void async_condition_variable::notify_one() {
    std::unique_lock<std::mutex> lock(m_lock);
    const auto awaiter = m_awaiters.pop_front();
    lock.unlock();

    if (awaiter != nullptr) {
        awaiter->resume();
    }
}

void async_condition_variable::notify_all() {
    std::unique_lock<std::mutex> lock(m_lock);
    auto awaiters = std::move(m_awaiters);
    lock.unlock();

    while (true) {
        const auto awaiter = awaiters.pop_front();
        if (awaiter == nullptr) {
            return;  // no more awaiters
        }

        awaiter->resume();
    }
}