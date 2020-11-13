#include "concurrencpp/results/impl/consumer_context.h"

#include "concurrencpp/results/executor_exception.h"
#include "concurrencpp/executors/executor.h"

using concurrencpp::details::await_context;
using concurrencpp::details::wait_context;
using concurrencpp::details::when_any_context;
using concurrencpp::details::consumer_context;

/*
 * await_context
 */

await_context::await_context(std::shared_ptr<executor> executor) noexcept : m_executor(std::move(executor)) {}

void await_context::set_coro_handle(std::experimental::coroutine_handle<> coro_handle) noexcept {
    m_handle = coro_handle;
}

void await_context::operator()(bool capture_executor_exception) {
    assert(static_cast<bool>(m_executor));
    assert(static_cast<bool>(m_handle));
    assert(static_cast<bool>(!m_handle.done()));

    try {
        m_executor->enqueue(m_handle);
    } catch (...) {
        if (!capture_executor_exception) {
            throw;
        }

        m_executor_exception = std::current_exception();
        m_handle();
    }
}

void await_context::throw_if_executor_threw() const {
    if (!static_cast<bool>(m_executor_exception)) {
        return;
    }

    throw concurrencpp::errors::executor_exception(m_executor_exception, m_executor);
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

when_any_context::when_any_context(std::shared_ptr<when_any_state_base> when_any_state, size_t index) noexcept :
    m_when_any_state(std::move(when_any_state)), m_index(index) {}

void when_any_context::operator()() const noexcept {
    assert(static_cast<bool>(m_when_any_state));
    m_when_any_state->on_result_ready(m_index);
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
            storage::destroy(m_storage.coro_handle);
            return;
        }

        case consumer_status::await_via: {
            storage::destroy(m_storage.await_ctx);
            return;
        }

        case consumer_status::wait: {
            storage::destroy(m_storage.wait_ctx);
            return;
        }

        case consumer_status::when_all: {
            storage::destroy(m_storage.when_all_state);
            return;
        }

        case consumer_status::when_any: {
            storage::destroy(m_storage.when_any_ctx);
            return;
        }
    }

    assert(false);
}

void consumer_context::set_await_context(std::experimental::coroutine_handle<> coro_handle) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await;
    storage::build(m_storage.coro_handle, coro_handle);
}

void consumer_context::set_await_via_context(await_context* await_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await_via;
    storage::build(m_storage.await_ctx, await_ctx);
}

void consumer_context::set_wait_context(std::shared_ptr<wait_context> wait_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::wait;
    storage::build(m_storage.wait_ctx, std::move(wait_ctx));
}

void consumer_context::set_when_all_context(std::shared_ptr<when_all_state_base> when_all_state) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::when_all;
    storage::build(m_storage.when_all_state, std::move(when_all_state));
}

void consumer_context::set_when_any_context(std::shared_ptr<when_any_state_base> when_any_ctx, size_t index) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::when_any;
    storage::build(m_storage.when_any_ctx, std::move(when_any_ctx), index);
}

void consumer_context::operator()() noexcept {
    switch (m_status) {
        case consumer_status::idle: {
            return;
        }

        case consumer_status::await: {
            return m_storage.coro_handle();
        }

        case consumer_status::await_via: {
            return (*m_storage.await_ctx)();
        }

        case consumer_status::wait: {
            return m_storage.wait_ctx->notify();
        }

        case consumer_status::when_all: {
            return m_storage.when_all_state->on_result_ready();
        }

        case consumer_status::when_any: {
            return m_storage.when_any_ctx();
        }
    }

    assert(false);
}
