#include "concurrencpp/results/impl/consumer_context.h"

#include "concurrencpp/executors/executor.h"
#include "concurrencpp/results/impl/shared_result_state.h"

using concurrencpp::details::when_any_context;
using concurrencpp::details::consumer_context;
using concurrencpp::details::await_via_functor;
using concurrencpp::details::result_state_base;

namespace concurrencpp::details {
    namespace {
        template<class type, class... argument_type>
        void build(type& o, argument_type&&... arguments) noexcept {
            new (std::addressof(o)) type(std::forward<argument_type>(arguments)...);
        }

        template<class type>
        void destroy(type& o) noexcept {
            o.~type();
        }
    }  // namespace
}  // namespace concurrencpp::details

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
 * when_any_context
 */

/*
 *   k_processing -> k_done_processing -> (completed) result_state_base*
 *     |                                             ^
 *     |                                             |
 *     ----------------------------------------------
 */

const result_state_base* when_any_context::k_processing = reinterpret_cast<result_state_base*>(-1);
const result_state_base* when_any_context::k_done_processing = nullptr;

when_any_context::when_any_context(coroutine_handle<void> coro_handle) noexcept : m_status(k_processing), m_coro_handle(coro_handle) {
    assert(static_cast<bool>(coro_handle));
    assert(!coro_handle.done());
}

bool when_any_context::any_result_finished() const noexcept {
    const auto status = m_status.load(std::memory_order_acquire);
    assert(status != k_done_processing);
    return status != k_processing;
}

bool when_any_context::finish_processing() noexcept {
    assert(m_status.load(std::memory_order_relaxed) != k_done_processing);

    // tries to turn k_processing -> k_done_processing.
    auto expected_state = k_processing;
    const auto res = m_status.compare_exchange_strong(expected_state, k_done_processing, std::memory_order_acq_rel);
    return res;  // if k_processing -> k_done_processing, then no result finished before the CAS, suspend.
}

void when_any_context::try_resume(result_state_base& completed_result) noexcept {
    /*
     * tries to turn m_status into the completed_result ptr
     * if m_status == k_processing, we just leave the pointer and bail out, the processor thread will pick
     *  the pointer up and resume from there
     * if m_status == k_done_processing AND we were able to CAS it into the completed result
     *   then we were the first ones to complete, processing is done for all input-results
     *   and we resume the caller
     */

    while (true) {
        auto status = m_status.load(std::memory_order_acquire);
        if (status != k_processing && status != k_done_processing) {
            return;  // another task finished before us, bail out
        }

        if (status == k_done_processing) {
            const auto swapped = m_status.compare_exchange_strong(status, &completed_result, std::memory_order_acq_rel);

            if (!swapped) {
                return;  // another task finished before us, bail out
            }

            // k_done_processing -> result_state_base ptr, we are the first to finish and CAS the status
            m_coro_handle();
            return;
        }

        assert(status == k_processing);
        const auto res = m_status.compare_exchange_strong(status, &completed_result, std::memory_order_acq_rel);

        if (res) {  // k_processing -> completed result_state_base*
            return;
        }

        // either another result raced us, either m_status is now k_done_processing, retry and act accordingly
    }
}

bool when_any_context::resume_inline(result_state_base& completed_result) noexcept {
    auto status = m_status.load(std::memory_order_acquire);
    assert(status != k_done_processing);

    if (status != k_processing) {
        return false;
    }

    // either we succeed turning k_processing to &completed_result, then we can resume inline, either we failed
    // meaning another thread had turned k_processing -> &completed_result, either way, testing if the cas succeeded
    // is redundant as we need to resume inline.
    m_status.compare_exchange_strong(status, &completed_result, std::memory_order_acq_rel);
    return false;
}

const result_state_base* when_any_context::completed_result() const noexcept {
    return m_status.load(std::memory_order_acquire);
}

/*
 * consumer_context
 */

consumer_context::~consumer_context() noexcept {
    destroy();
}

void consumer_context::destroy() noexcept {
    switch (m_status) {
        case consumer_status::idle: {
            return;
        }

        case consumer_status::await: {
            return details::destroy(m_storage.caller_handle);
        }

        case consumer_status::wait_for: {
            return details::destroy(m_storage.wait_for_ctx);
        }

        case consumer_status::when_any: {
            return details::destroy(m_storage.when_any_ctx);
        }

        case consumer_status::shared: {
            return details::destroy(m_storage.shared_ctx);
        }
    }

    assert(false);
}

void consumer_context::clear() noexcept {
    destroy();
    m_status = consumer_status::idle;
}

void consumer_context::set_await_handle(coroutine_handle<void> caller_handle) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::await;
    details::build(m_storage.caller_handle, caller_handle);
}

void consumer_context::set_wait_for_context(const std::shared_ptr<std::binary_semaphore>& wait_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::wait_for;
    details::build(m_storage.wait_for_ctx, wait_ctx);
}

void consumer_context::set_when_any_context(const std::shared_ptr<when_any_context>& when_any_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::when_any;
    details::build(m_storage.when_any_ctx, when_any_ctx);
}

void concurrencpp::details::consumer_context::set_shared_context(const std::shared_ptr<shared_result_state_base>& shared_ctx) noexcept {
    assert(m_status == consumer_status::idle);
    m_status = consumer_status::shared;
    details::build(m_storage.shared_ctx, shared_ctx);
}

void consumer_context::resume_consumer(result_state_base& self) const {
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
            return wait_ctx->release();
        }

        case consumer_status::when_any: {
            const auto when_any_ctx = m_storage.when_any_ctx;
            return when_any_ctx->try_resume(self);
        }

        case consumer_status::shared: {
            const auto weak_shared_ctx = m_storage.shared_ctx;
            const auto shared_ctx = weak_shared_ctx.lock();
            if (static_cast<bool>(shared_ctx)) {
                shared_ctx->on_result_finished();
            }
            return;
        }
    }

    assert(false);
}