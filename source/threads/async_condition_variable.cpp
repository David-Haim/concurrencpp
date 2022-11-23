#include "concurrencpp/threads/constants.h"
#include "concurrencpp/threads/async_condition_variable.h"

using concurrencpp::executor;
using concurrencpp::lazy_result;
using concurrencpp::scoped_async_lock;
using concurrencpp::async_condition_variable;

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