#include "concurrencpp/results/resume_on.h"
#include "concurrencpp/threads/constants.h"
#include "concurrencpp/threads/async_condition_variable.h"

using concurrencpp::executor;
using concurrencpp::lazy_result;
using concurrencpp::scoped_async_lock;
using concurrencpp::async_condition_variable;

namespace concurrencpp::details {
    class CRCPP_API cv_awaitable {
       private:
        async_condition_variable& m_parent;
        scoped_async_lock& m_lock;
        details::coroutine_handle<void> m_caller_handle;

       public:
        cv_awaitable* next = nullptr;

        cv_awaitable(async_condition_variable& parent, scoped_async_lock& lock) noexcept : m_parent(parent), m_lock(lock) {}

        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(details::coroutine_handle<void> caller_handle) {
            m_caller_handle = caller_handle;

            std::unique_lock<std::mutex> lock(m_parent.m_lock);
            m_lock.unlock();

            m_parent.m_awaiters.push_back(this);
        }

        void await_resume() const noexcept {}

        void resume() noexcept {
            assert(static_cast<bool>(m_caller_handle));
            assert(!m_caller_handle.done());
            m_caller_handle();
        }
    };
}  // namespace concurrencpp::details

/*
    async_condition_variable::slist
*/

async_condition_variable::slist::slist(slist&& rhs) noexcept :
    m_head(std::exchange(rhs.m_head, nullptr)), m_tail(std::exchange(rhs.m_tail, nullptr)) {}

bool async_condition_variable::slist::empty() const noexcept {
    return m_head == nullptr;
}

void async_condition_variable::slist::push_back(details::cv_awaitable* node) noexcept {
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

concurrencpp::details::cv_awaitable* async_condition_variable::slist::pop_front() noexcept {
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
    co_await details::cv_awaitable(*this, lock);
    assert(!lock.owns_lock());
    co_await resume_on(resume_executor);  // TODO: optimize this when get_current_executor is available
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