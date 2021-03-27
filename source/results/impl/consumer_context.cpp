#include "concurrencpp/results/impl/consumer_context.h"
#include "concurrencpp/results/impl/shared_result_state.h"

#include "concurrencpp/executors/executor.h"

using concurrencpp::details::await_via_context;
using concurrencpp::details::wait_context;
using concurrencpp::details::when_any_context;
using concurrencpp::details::consumer_context;
using concurrencpp::details::await_via_functor;

/*
 * await_via_context
 */

void await_via_context::await_context::resume() noexcept {
    assert(static_cast<bool>(handle));
    assert(!handle.done());
    handle();
}

void await_via_context::await_context::set_coro_handle(coroutine_handle<void> coro_handle) noexcept {
    assert(!static_cast<bool>(handle));
    assert(static_cast<bool>(coro_handle));
    assert(!coro_handle.done());
    handle = coro_handle;
}

void await_via_context::await_context::set_interrupt(const std::exception_ptr& interrupt) noexcept {
    assert(interrupt_exception == nullptr);
    assert(static_cast<bool>(interrupt));
    interrupt_exception = interrupt;
}

void await_via_context::await_context::throw_if_interrupted() const {
    if (interrupt_exception != nullptr) {
        std::rethrow_exception(interrupt_exception);
    }
}

await_via_context::await_via_context(const std::shared_ptr<executor>& executor) noexcept : m_executor(executor) {}

void await_via_context::resume() noexcept {
    m_await_ctx.resume();
}

void await_via_context::operator()() noexcept {
    try {
        m_executor->enqueue(get_functor());
    } catch (...) {
        // If an exception is thrown, the task destructor sets an interrupt exception
        // on the await_context and resumes the coroutine, causing a broken_task exception to be thrown.
        // this is why we don't let this exception propagate, as it wil cause the coroutine to be resumed twice (UB)
    }
}

void await_via_context::set_coro_handle(coroutine_handle<void> coro_handle) noexcept {
    m_await_ctx.set_coro_handle(coro_handle);
}

void await_via_context::set_interrupt(const std::exception_ptr& interrupt) noexcept {
    m_await_ctx.set_interrupt(interrupt);
}

void await_via_context::throw_if_interrupted() const {
    m_await_ctx.throw_if_interrupted();
}

await_via_functor await_via_context::get_functor() noexcept {
    return {&m_await_ctx};
}

/*
 * await_via_functor
 */

await_via_functor::await_via_functor(await_via_context::await_context* ctx) noexcept : m_ctx(ctx) {}

await_via_functor::await_via_functor(await_via_functor&& rhs) noexcept : m_ctx(rhs.m_ctx) {
    rhs.m_ctx = nullptr;
}

await_via_functor ::~await_via_functor() noexcept {
    if (m_ctx == nullptr) {
        return;
    }

    m_ctx->set_interrupt(std::make_exception_ptr(errors::broken_task(consts::k_broken_task_exception_error_msg)));
    m_ctx->resume();
}

void await_via_functor::operator()() noexcept {
    assert(m_ctx != nullptr);
    const auto await_context = std::exchange(m_ctx, nullptr);
    await_context->resume();
}

/*
 * wait_context
 */

void wait_context::wait() noexcept {
    std::unique_lock<std::mutex> lock(m_lock);
    m_condition.wait(lock, [this] {
        return m_ready;
    });
}

bool wait_context::wait_for(size_t milliseconds) noexcept {
    std::unique_lock<std::mutex> lock(m_lock);
    return m_condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] {
        return m_ready;
    });
}

void wait_context::notify() noexcept {
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_ready = true;
    }

    m_condition.notify_all();
}

/*
 * when_any_context
 */

when_any_context::when_any_context(const std::shared_ptr<when_any_state_base>& when_any_state, size_t index) noexcept :
    m_when_any_state(std::move(when_any_state)), m_index(index) {}

void when_any_context::operator()() const noexcept {
    const auto when_any_state = m_when_any_state;
    const auto index = m_index;

    assert(static_cast<bool>(when_any_state));
    when_any_state->on_result_ready(index);
}

/*
 * consumer_context
 */

consumer_context::consumer_context() noexcept : m_status(consumer_status::idle) {}

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

        case consumer_status::await_via: {
            storage::destroy(m_storage.await_via_ctx);
            return;
        }

        case consumer_status::wait: {
            storage::destroy(m_storage.wait_ctx);
            return;
        }

        case consumer_status::when_all: {
            storage::destroy(m_storage.when_all_ctx);
            return;
        }

        case consumer_status::when_any: {
            storage::destroy(m_storage.when_any_ctx);
            return;
        }

        case consumer_status::shared: {
            storage::destroy(m_storage.shared_ctx);
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

void consumer_context::set_await_via_context(await_via_context& await_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await_via;
    storage::build(m_storage.await_via_ctx, std::addressof(await_ctx));
}

void consumer_context::set_wait_context(const std::shared_ptr<wait_context>& wait_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::wait;
    storage::build(m_storage.wait_ctx, wait_ctx);
}

void consumer_context::set_when_all_context(const std::shared_ptr<when_all_state_base>& when_all_state) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::when_all;
    storage::build(m_storage.when_all_ctx, when_all_state);
}

void consumer_context::set_when_any_context(const std::shared_ptr<when_any_state_base>& when_any_ctx, size_t index) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::when_any;
    storage::build(m_storage.when_any_ctx, when_any_ctx, index);
}

void consumer_context::set_shared_context(const std::weak_ptr<shared_result_state_base>& shared_result_state) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::shared;
    storage::build(m_storage.shared_ctx, shared_result_state);
}

void consumer_context::resume_consumer() const noexcept {
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

        case consumer_status::await_via: {
            const auto await_via_ctx_ptr = m_storage.await_via_ctx;
            assert(await_via_ctx_ptr != nullptr);
            return (*await_via_ctx_ptr)();
        }

        case consumer_status::wait: {
            const auto wait_ctx = m_storage.wait_ctx;
            assert(static_cast<bool>(wait_ctx));
            return wait_ctx->notify();
        }

        case consumer_status::when_all: {
            const auto when_all_ctx = m_storage.when_all_ctx;
            assert(static_cast<bool>(when_all_ctx));
            return when_all_ctx->on_result_ready();
        }

        case consumer_status::when_any: {
            const auto when_any_ctx = m_storage.when_any_ctx;
            return when_any_ctx();
        }

        case consumer_status::shared: {
            const auto shared_state = m_storage.shared_ctx.lock();
            if (static_cast<bool>(shared_state)) {
                shared_state->on_result_ready();
            }

            return;
        }
    }

    assert(false);
}