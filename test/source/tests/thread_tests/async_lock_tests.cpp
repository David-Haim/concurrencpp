#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/executor_shutdowner.h"

#include "concurrencpp/threads/constants.h"

namespace concurrencpp::tests {
    void test_async_lock_lock_null_resume_executor();
    void test_async_lock_lock_resumption();
    void test_async_lock_lock();

    void test_async_lock_try_lock();

    void test_async_lock_unlock_resumption_fails();
    void test_async_lock_unlock();

    void test_async_lock_mini_load_test1();
    void test_async_lock_mini_load_test2();
    void test_async_lock_lock_unlock();

    result<void> incremenet(executor_tag, std::shared_ptr<executor> ex, async_lock& lock, size_t& counter, size_t cycles) {
        for (size_t i = 0; i < cycles; i++) {
            auto lk = co_await lock.lock(ex);
            ++counter;
        }
    }

    result<void> insert(executor_tag,
                        std::shared_ptr<executor> ex,
                        async_lock& lock,
                        std::vector<size_t>& vec,
                        size_t range_begin,
                        size_t range_end) {
        for (size_t i = range_begin; i < range_end; i++) {
            auto lk = co_await lock.lock(ex);
            vec.emplace_back(i);
        }
    }

    result<std::pair<uintptr_t, uintptr_t>> get_thread_ids(async_lock& lock, std::shared_ptr<worker_thread_executor> executor) {
        const auto before_id = concurrencpp::details::thread::get_current_virtual_id();
        auto lock_guard = co_await lock.lock(executor);
        const auto after_id = concurrencpp::details::thread::get_current_virtual_id();
        co_return std::make_pair(before_id, after_id);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_async_lock_lock_null_resume_executor() {
    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            async_lock lock;
            lock.lock(std::shared_ptr<concurrencpp::inline_executor> {});
        },
        concurrencpp::details::consts::k_async_lock_null_resume_executor_err_msg);
}

void concurrencpp::tests::test_async_lock_lock_resumption() {
    async_lock lock;
    const auto worker_thread = std::make_shared<worker_thread_executor>();
    const auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es0(worker_thread), es1(thread_executor);

    // if async_mutex is available for acquiring the coroutine resumes inline
    {
        const auto [before, after] = get_thread_ids(lock, worker_thread).get();
        assert_equal(before, after);
    }

    // if async_mutex nor is available for acquiring, the coroutine resumes inside resume_executor
    {
        auto guard = lock.lock(worker_thread).run().get();
        auto ids = thread_executor->submit(get_thread_ids, std::ref(lock), worker_thread).get();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        guard.unlock();
        const auto [before, after] = ids.get();
        assert_not_equal(before, after);
    }
}

void concurrencpp::tests::test_async_lock_lock() {
    test_async_lock_lock_null_resume_executor();
    test_async_lock_lock_resumption();
}

void concurrencpp::tests::test_async_lock_try_lock() {
    async_lock lock;
    assert_true(lock.try_lock().run().get());
    assert_false(lock.try_lock().run().get());

    lock.unlock();
    assert_true(lock.try_lock().run().get());

    lock.unlock();
}

namespace concurrencpp::tests {
    result<void> lock_coro(async_lock& lock, std::shared_ptr<executor> ex) {
        auto g = co_await lock.lock(ex);
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_async_lock_unlock_resumption_fails() {
    /* let's say that one coroutine tried to lock a lock and failed because the lock is already locked.
     * that coroutine was queued for resumption for when async_lock::unlock is called.
     * when unlock is called, the coroutine manages to lock the lock but executor::shutdown is called
     * on the resume-executor.
     * In this case, another coroutine must be resumed instead, otherwise the chain of locking/unlocking will be lost.
     */

    runtime runtime;
    std::shared_ptr<worker_thread_executor> executors[5];
    std::shared_ptr<worker_thread_executor> working_executor = runtime.make_worker_thread_executor();

    result<void> results[5];
    result<void> result;

    for (auto& executor : executors) {
        executor = runtime.make_worker_thread_executor();
    }

    async_lock lock;

    auto g = lock.lock(runtime.thread_pool_executor()).run().get();

    for (size_t i = 0; i < std::size(executors); i++) {
        results[i] = lock_coro(lock, executors[i]);
    }

    result = lock_coro(lock, working_executor);

    for (auto& executor : executors) {
        executor->shutdown();
    }

    g.unlock();

    for (auto& err_result : results) {
        assert_throws<errors::broken_task>([&err_result] {
            err_result.get();
        });
    }

    result.get();  // make sure nothing is thrown
}

void concurrencpp::tests::test_async_lock_unlock() {
    // unlocking an un-owned lock throws
    assert_throws_contains_error_message<std::system_error>(
        [] {
            async_lock lock;
            lock.unlock();
        },
        concurrencpp::details::consts::k_async_lock_unlock_invalid_lock_err_msg);

    test_async_lock_unlock_resumption_fails();
}

void concurrencpp::tests::test_async_lock_mini_load_test1() {
    async_lock mtx;
    size_t counter = 0;

    const size_t worker_count = concurrencpp::details::thread::hardware_concurrency();
    constexpr size_t cycles = 100'000;

    std::vector<std::shared_ptr<worker_thread_executor>> workers(worker_count);
    for (auto& worker : workers) {
        worker = std::make_shared<worker_thread_executor>();
    }

    std::vector<result<void>> results(worker_count);

    for (size_t i = 0; i < worker_count; i++) {
        results[i] = incremenet({}, workers[i], mtx, counter, cycles);
    }

    for (size_t i = 0; i < worker_count; i++) {
        results[i].get();
    }

    {
        auto lock = mtx.lock(workers[0]).run().get();
        assert_equal(counter, cycles * worker_count);
    }

    for (auto& worker : workers) {
        worker->shutdown();
    }
}

void concurrencpp::tests::test_async_lock_mini_load_test2() {
    async_lock mtx;
    std::vector<size_t> vector;

    const size_t worker_count = concurrencpp::details::thread::hardware_concurrency();
    constexpr size_t cycles = 100'000;

    std::vector<std::shared_ptr<worker_thread_executor>> workers(worker_count);
    for (auto& worker : workers) {
        worker = std::make_shared<worker_thread_executor>();
    }

    std::vector<result<void>> results(worker_count);

    for (size_t i = 0; i < worker_count; i++) {
        results[i] = insert({}, workers[i], mtx, vector, i * cycles, (i + 1) * cycles);
    }

    for (size_t i = 0; i < worker_count; i++) {
        results[i].get();
    }

    {
        auto lock = mtx.lock(workers[0]).run().get();
        assert_equal(vector.size(), cycles * worker_count);

        std::sort(vector.begin(), vector.end());

        for (size_t i = 0; i < worker_count * cycles; i++) {
            assert_equal(vector[i], i);
        }
    }

    for (auto& worker : workers) {
        worker->shutdown();
    }
}

void concurrencpp::tests::test_async_lock_lock_unlock() {
    test_async_lock_mini_load_test1();
    test_async_lock_mini_load_test2();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("async_lock test");

    tester.add_step("lock", test_async_lock_lock);
    tester.add_step("try_lock", test_async_lock_try_lock);
    tester.add_step("unlock", test_async_lock_unlock);
    tester.add_step("lock + unlock", test_async_lock_lock_unlock);

    tester.launch_test();
    return 0;
}