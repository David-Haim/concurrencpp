#include "concurrencpp/results/impl/consumer_context.h"
#include "concurrencpp/results/impl/shared_result_state.h"

#include "concurrencpp/executors/executor.h"

using concurrencpp::details::await_context;
using concurrencpp::details::await_via_context;
using concurrencpp::details::wait_context;
using concurrencpp::details::when_any_context;
using concurrencpp::details::consumer_context;

namespace concurrencpp::details {
    class await_context_wrapper {

       private:
        await_context* m_await_context;

       public:
        await_context_wrapper(await_context* await_context) noexcept : m_await_context(await_context) {}

        await_context_wrapper(await_context_wrapper&& rhs) noexcept : m_await_context(rhs.m_await_context) {
            rhs.m_await_context = nullptr;
        }

        ~await_context_wrapper() noexcept {
            if (m_await_context == nullptr) {
                return;
            }

            m_await_context->set_interrupt(std::make_exception_ptr(errors::broken_task(consts::k_broken_task_exception_error_msg)));
            (*m_await_context)();
        }

        void operator()() noexcept {
            assert(m_await_context != nullptr);
            auto await_context = std::exchange(m_await_context, nullptr);
            (*await_context)();
        }
    };
}  // namespace concurrencpp::details

/*
 * await_context
 */

void await_context::set_coro_handle(details::coroutine_handle<void> coro_handle) noexcept {
    assert(!static_cast<bool>(m_handle));
    assert(static_cast<bool>(coro_handle));
    assert(!coro_handle.done());
    m_handle = coro_handle;
}

void await_context::set_interrupt(const std::exception_ptr& interrupt) {
    assert(m_interrupt_exception == nullptr);
    assert(static_cast<bool>(interrupt));
    m_interrupt_exception = interrupt;
}

void await_context::operator()() noexcept {
    assert(static_cast<bool>(m_handle));
    assert(!m_handle.done());
    m_handle();
}

void await_context::throw_if_interrupted() const {
    if (m_interrupt_exception == nullptr) {
        return;
    }

    std::rethrow_exception(m_interrupt_exception);
}

concurrencpp::task await_context::to_task() noexcept {
    return concurrencpp::task {await_context_wrapper {this}};
}

/*
 * await_via_context
 */

await_via_context::await_via_context(std::shared_ptr<executor> executor) noexcept : m_executor(std::move(executor)) {}

void await_via_context::set_coro_handle(details::coroutine_handle<void> coro_handle) noexcept {
    m_await_context.set_coro_handle(coro_handle);
}

void await_via_context::operator()() noexcept {
    try {
        m_executor->enqueue(m_await_context.to_task());
    } catch (...) {
        // If an exception is thrown, the task destructor sets an interrupt exception
        // on the await_context and resumes the coroutine, causing a broken_task exception to be thrown.
        // this is why we don't let this exception propagate, as it wil cause the coroutine to be resumed twice (UB)
    }
}
void await_via_context::throw_if_interrupted() const {
    m_await_context.throw_if_interrupted();
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
            storage::destroy(m_storage.await_context);
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
            storage::destroy(m_storage.when_all_state);
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

void consumer_context::set_await_context(await_context* await_context) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await;
    storage::build(m_storage.await_context, await_context);
}

void consumer_context::set_await_via_context(await_via_context* await_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await_via;
    storage::build(m_storage.await_via_ctx, await_ctx);
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

void consumer_context::set_shared_context(std::weak_ptr<shared_result_state_base> shared_result_state) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::shared;
    storage::build(m_storage.shared_ctx, std::move(shared_result_state));
}

void consumer_context::operator()() noexcept {
    switch (m_status) {
        case consumer_status::idle: {
            return;
        }

        case consumer_status::await: {
            return (*m_storage.await_context)();
        }

        case consumer_status::await_via: {
            return (*m_storage.await_via_ctx)();
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

        case consumer_status::shared: {
            auto shared_state = m_storage.shared_ctx.lock();
            if (static_cast<bool>(shared_state)) {
                shared_state->notify_all();
            }
            return;
        }
    }

    assert(false);
}
