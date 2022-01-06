#include "concurrencpp/results/impl/consumer_context.h"

#include "concurrencpp/executors/executor.h"

using concurrencpp::details::wait_context;
using concurrencpp::details::when_any_context;
using concurrencpp::details::consumer_context;
using concurrencpp::details::await_via_functor;

/*
 * await_via_functor
 */

await_via_functor::await_via_functor(coroutine_handle<void> caller_handle, bool* interrupted) noexcept :
    m_caller_handle(caller_handle), m_interrupted(interrupted) {
    assert(static_cast<bool>(caller_handle));
    assert(!caller_handle.done());
    assert(interrupted != nullptr);
}

await_via_functor::await_via_functor(await_via_functor&& rhs) noexcept :
    m_caller_handle(std::exchange(rhs.m_caller_handle, {})), m_interrupted(std::exchange(rhs.m_interrupted, nullptr)) {}

await_via_functor ::~await_via_functor() noexcept {
    if (m_interrupted == nullptr) {
        return;
    }

    *m_interrupted = true;
    m_caller_handle();
}

void await_via_functor::operator()() noexcept {
    assert(m_interrupted != nullptr);
    m_interrupted = nullptr;
    m_caller_handle();
}

/*
 * wait_context
 */

void wait_context::wait() {
    std::unique_lock<std::mutex> lock(m_lock);
    m_condition.wait(lock, [this] {
        return m_ready;
    });
}

bool wait_context::wait_for(size_t milliseconds) {
    std::unique_lock<std::mutex> lock(m_lock);
    return m_condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] {
        return m_ready;
    });
}

void wait_context::notify() {
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_ready = true;
    }

    m_condition.notify_all();
}

/*
 * when_any_context
 */

when_any_context::when_any_context(coroutine_handle<void> coro_handle) noexcept : m_coro_handle(coro_handle) {}

void when_any_context::try_resume(result_state_base* completed_result) noexcept {
    assert(completed_result != nullptr);

    const auto already_resumed = m_fulfilled.exchange(true, std::memory_order_acq_rel);
    if (already_resumed) {
        return;
    }

    assert(m_completed_result == nullptr);
    m_completed_result = completed_result;

    assert(static_cast<bool>(m_coro_handle));
    m_coro_handle();
}

bool when_any_context::fulfilled() const noexcept {
    return m_fulfilled.load(std::memory_order_acquire);
}

concurrencpp::details::result_state_base* when_any_context::completed_result() const noexcept {
    assert(m_completed_result != nullptr);
    return m_completed_result;
}

/*
 * consumer_context
 */

consumer_context::~consumer_context() noexcept {
    clear();
}

void consumer_context::clear() noexcept {
    const auto status = std::exchange(m_status, consumer_status::idle);

    switch (status) {
        case consumer_status::idle: {
            return;
        }

        case consumer_status::await: {
            storage::destroy(m_storage.caller_handle);
            return;
        }

        case consumer_status::wait_for: {
            storage::destroy(m_storage.wait_for_ctx);
            return;
        }

        case consumer_status::when_any: {
            storage::destroy(m_storage.when_any_ctx);
            return;
        }
    }

    assert(false);
}

void consumer_context::set_await_handle(coroutine_handle<void> caller_handle) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await;
    storage::build(m_storage.caller_handle, caller_handle);
}

void consumer_context::set_wait_for_context(const std::shared_ptr<wait_context>& wait_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::wait_for;
    storage::build(m_storage.wait_for_ctx, wait_ctx);
}

void consumer_context::set_when_any_context(const std::shared_ptr<when_any_context>& when_any_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::when_any;
    storage::build(m_storage.when_any_ctx, when_any_ctx);
}

void consumer_context::resume_consumer(result_state_base* self) const {
    switch (m_status) {
        case consumer_status::idle: {
            return;
        }

        case consumer_status::await: {
            auto caller_handle = m_storage.caller_handle;
            assert(static_cast<bool>(caller_handle));
            assert(!caller_handle.done());
            return caller_handle();
        }

        case consumer_status::wait_for: {
            const auto wait_ctx = m_storage.wait_for_ctx;
            assert(static_cast<bool>(wait_ctx));
            return wait_ctx->notify();
        }

        case consumer_status::when_any: {
            const auto when_any_ctx = m_storage.when_any_ctx;
            return when_any_ctx->try_resume(self);
        }
    }

    assert(false);
}