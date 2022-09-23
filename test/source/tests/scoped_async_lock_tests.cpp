#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/executor_shutdowner.h"

#include "concurrencpp/threads/constants.h"

namespace concurrencpp::tests {
    result<void> test_scoped_async_lock_constructor_impl(std::shared_ptr<executor> executor);
    void test_scoped_async_lock_constructor();

    result<void> test_scoped_async_lock_destructor_impl(std::shared_ptr<executor> executor);
    void test_scoped_async_lock_destructor();

    void test_scoped_async_lock_lock();
    void test_scoped_async_lock_try_lock();
    void test_scoped_async_lock_unlock();

    void test_scoped_async_lock_swap();
    void test_scoped_async_lock_release();

}  // namespace concurrencpp::tests

concurrencpp::result<void> concurrencpp::tests::test_scoped_async_lock_constructor_impl(std::shared_ptr<executor> executor) {
    {  // default constructor
        scoped_async_lock sal;
        assert_false(sal.owns_lock());
        assert_false(static_cast<bool>(sal));
        assert_equal(sal.mutex(), nullptr);
    }

    {  // move constructor
        async_lock lock;
        scoped_async_lock sal = co_await lock.lock(executor);
        assert_true(sal.owns_lock());
        assert_true(static_cast<bool>(sal));
        assert_equal(sal.mutex(), &lock);

        auto sal1 = std::move(sal);
        assert_false(sal.owns_lock());
        assert_false(static_cast<bool>(sal));
        assert_equal(sal.mutex(), nullptr);

        assert_true(sal1.owns_lock());
        assert_true(static_cast<bool>(sal1));
        assert_equal(sal1.mutex(), &lock);

        // moving an unowned mutex
        scoped_async_lock empty;
        auto sal2 = std::move(empty);
        assert_false(sal2.owns_lock());
        assert_false(static_cast<bool>(sal2));
        assert_equal(sal2.mutex(), nullptr);
    }

    {
        async_lock lock;
        scoped_async_lock sal(lock, std::defer_lock);
        assert_false(sal.owns_lock());
        assert_false(static_cast<bool>(sal));
        assert_equal(sal.mutex(), &lock);
    }

    {
        async_lock lock;
        const auto locked = co_await lock.try_lock();
        assert_true(locked);
        scoped_async_lock sal(lock, std::adopt_lock);
        assert_true(sal.owns_lock());
        assert_true(static_cast<bool>(sal));
        assert_equal(sal.mutex(), &lock);
    }
}

void concurrencpp::tests::test_scoped_async_lock_constructor() {
    auto worker = std::make_shared<concurrencpp::worker_thread_executor>();
    executor_shutdowner es(worker);
    test_scoped_async_lock_constructor_impl(worker).get();
}

concurrencpp::result<void> concurrencpp::tests::test_scoped_async_lock_destructor_impl(std::shared_ptr<executor> executor) {
    async_lock lock;

    {  // non-owning scoped_async_lock
        scoped_async_lock sal(lock, std::defer_lock);
    }

    {  // empty scoped_async_lock
        auto sal = co_await lock.lock(executor);
        auto mtx = sal.release();
        mtx->unlock();
    }
}

void concurrencpp::tests::test_scoped_async_lock_destructor() {
    const auto worker = std::make_shared<concurrencpp::worker_thread_executor>();
    executor_shutdowner es(worker);
    test_scoped_async_lock_destructor_impl(worker);
}

void concurrencpp::tests::test_scoped_async_lock_lock() {
    auto executor = std::make_shared<concurrencpp::worker_thread_executor>();
    executor_shutdowner es(executor);

    {  // null resume_executor
        assert_throws_contains_error_message<std::invalid_argument>(
            [] {
                async_lock lock;
                scoped_async_lock sal(lock, std::defer_lock);
                sal.lock(std::shared_ptr<concurrencpp::executor>()).run().get();
            },
            concurrencpp::details::consts::k_scoped_async_lock_null_resume_executor_err_msg);
    }

    {  // empty scoped_async_lock
        assert_throws_contains_error_message<std::system_error>(
            [executor] {
                scoped_async_lock sal;
                sal.lock(executor).run().get();
            },
            concurrencpp::details::consts::k_scoped_async_lock_lock_no_mutex_err_msg);
    }

    {  // lock would cause a deadlock
        assert_throws_contains_error_message<std::system_error>(
            [executor] {
                async_lock lock;
                auto sal = lock.lock(executor).run().get();
                sal.lock(executor).run().get();
            },
            concurrencpp::details::consts::k_scoped_async_lock_lock_deadlock_err_msg);
    }

    // valid scenario
    async_lock lock;
    scoped_async_lock sal(lock, std::defer_lock);
    assert_false(sal.owns_lock());

    sal.lock(executor).run().get();
    assert_true(sal.owns_lock());
}

void concurrencpp::tests::test_scoped_async_lock_try_lock() {
    {  // empty scoped_async_lock
        assert_throws_contains_error_message<std::system_error>(
            [] {
                scoped_async_lock sal;
                sal.try_lock().run().get();
            },
            concurrencpp::details::consts::k_scoped_async_lock_try_lock_no_mutex_err_msg);
    }

    {  // lock would cause a deadlock
        assert_throws_contains_error_message<std::system_error>(
            [] {
                async_lock lock;
                const auto locked = lock.try_lock().run().get();
                assert_true(locked);

                scoped_async_lock sal(lock, std::adopt_lock);
                sal.try_lock().run().get();
            },
            concurrencpp::details::consts::k_scoped_async_lock_try_lock_deadlock_err_msg);
    }

    // valid scenario
    async_lock lock;
    scoped_async_lock sal(lock, std::defer_lock);
    assert_false(sal.owns_lock());

    const auto locked = sal.try_lock().run().get();
    assert_true(locked);

    assert_true(sal.owns_lock());
}

void concurrencpp::tests::test_scoped_async_lock_unlock() {
    async_lock lock;
    scoped_async_lock sal(lock, std::defer_lock);

    assert_throws_contains_error_message<std::system_error>(
        [&sal] {
            sal.unlock();
        },
        concurrencpp::details::consts::k_scoped_async_lock_unlock_invalid_lock_err_msg);

    const auto locked = sal.try_lock().run().get();
    assert_true(locked);

    sal.unlock();
    assert_false(sal.owns_lock());
    assert_false(static_cast<bool>(sal));
    assert_equal(sal.mutex(), &lock);
}

void concurrencpp::tests::test_scoped_async_lock_swap() {
    {  // empty + empty
        scoped_async_lock sal0, sal1;
        sal0.swap(sal1);

        assert_false(sal0.owns_lock());
        assert_false(static_cast<bool>(sal0));
        assert_equal(sal0.mutex(), nullptr);

        assert_false(sal1.owns_lock());
        assert_false(static_cast<bool>(sal1));
        assert_equal(sal1.mutex(), nullptr);
    }

    {  // empty + non-empty
        async_lock lock1;
        const auto locked = lock1.try_lock().run().get();
        assert_true(locked);

        scoped_async_lock sal0, sal1(lock1, std::adopt_lock);
        sal0.swap(sal1);

        assert_true(sal0.owns_lock());
        assert_true(static_cast<bool>(sal0));
        assert_equal(sal0.mutex(), &lock1);

        assert_false(sal1.owns_lock());
        assert_false(static_cast<bool>(sal1));
        assert_equal(sal1.mutex(), nullptr);
    }

    {  // non-empty + empty
        async_lock lock0;
        const auto locked = lock0.try_lock().run().get();
        assert_true(locked);

        scoped_async_lock sal0(lock0, std::adopt_lock), sal1;
        sal0.swap(sal1);

        assert_false(sal0.owns_lock());
        assert_false(static_cast<bool>(sal0));
        assert_equal(sal0.mutex(), nullptr);

        assert_true(sal1.owns_lock());
        assert_true(static_cast<bool>(sal1));
        assert_equal(sal1.mutex(), &lock0);
    }

    {  // non-empty + non-empty
        async_lock lock0, lock1;
        const auto locked0 = lock0.try_lock().run().get();
        assert_true(locked0);

        const auto locked1 = lock1.try_lock().run().get();
        assert_true(locked1);

        scoped_async_lock sal0(lock0, std::adopt_lock), sal1(lock1, std::adopt_lock);
        sal0.swap(sal1);

        assert_true(sal0.owns_lock());
        assert_true(static_cast<bool>(sal0));
        assert_equal(sal0.mutex(), &lock1);

        assert_true(sal1.owns_lock());
        assert_true(static_cast<bool>(sal1));
        assert_equal(sal1.mutex(), &lock0);
    }

    {  // swap with self
        async_lock lock;
        const auto locked = lock.try_lock().run().get();
        assert_true(locked);

        scoped_async_lock sal(lock, std::adopt_lock);
        sal.swap(sal);

        assert_true(sal.owns_lock());
        assert_true(static_cast<bool>(sal));
        assert_equal(sal.mutex(), &lock);
    }
}

void concurrencpp::tests::test_scoped_async_lock_release() {
    // empty:
    scoped_async_lock sal;
    assert_equal(sal.release(), nullptr);

    async_lock lock;
    const auto locked = lock.try_lock().run().get();
    assert_true(locked);
    scoped_async_lock sal0(lock, std::adopt_lock);

    const auto lock_ptr = sal0.release();
    assert_equal(lock_ptr, &lock);
    lock_ptr->unlock();

    assert_false(sal0.owns_lock());
    assert_false(static_cast<bool>(sal0));
    assert_equal(sal0.mutex(), nullptr);
}

using namespace concurrencpp::tests;

int main() {
    tester tester("scoped_async_lock test");

    tester.add_step("constructor", test_scoped_async_lock_constructor);
    tester.add_step("destructor", test_scoped_async_lock_destructor);
    tester.add_step("lock", test_scoped_async_lock_lock);
    tester.add_step("try_lock", test_scoped_async_lock_try_lock);
    tester.add_step("unlock", test_scoped_async_lock_unlock);
    tester.add_step("swap", test_scoped_async_lock_swap);
    tester.add_step("release", test_scoped_async_lock_release);

    tester.launch_test();
    return 0;
}