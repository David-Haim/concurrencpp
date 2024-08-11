#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"

namespace concurrencpp::tests {
    void test_atomic_wait();

    void test_atomic_wait_for_timeout_1();
    void test_atomic_wait_for_timeout_2();
    void test_atomic_wait_for_success();
    void test_atomic_wait_for();

    void test_atomic_notify_one();
    void test_atomic_notify_all();

    void test_atomic_mini_load_test();
}  // namespace concurrencpp::tests

using namespace concurrencpp::tests;

void concurrencpp::tests::test_atomic_wait() {
    std::atomic_uint32_t flag {0};
    std::atomic_bool woken {false};

    std::thread waiter([&] {
        concurrencpp::details::atomic_wait(flag, uint32_t(0), std::memory_order_acquire);
        woken = true;
    });

    for (size_t i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert_false(woken.load());
    }

    // notify was called, but value hadn't changed
    for (size_t i = 0; i < 5; i++) {
        concurrencpp::details::atomic_notify_all(flag);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        assert_false(woken.load());
    }

    // value had changed + notify was called
    flag = 1;
    concurrencpp::details::atomic_notify_all(flag);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    assert_true(woken.load());

    waiter.join();
}

void concurrencpp::tests::test_atomic_wait_for_timeout_1() {
    // timeout has reached
    std::atomic_uint32_t flag {0};
    constexpr auto timeout_ms = 350;

    const auto before = std::chrono::high_resolution_clock::now();
    const auto result =
        concurrencpp::details::atomic_wait_for(flag, uint32_t(0), std::chrono::milliseconds(timeout_ms), std::memory_order_acquire);
    const auto after = std::chrono::high_resolution_clock::now();
    const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();

    assert_equal(result, concurrencpp::details::atomic_wait_status::timeout);
    assert_bigger_equal(time_diff, timeout_ms);
}

void concurrencpp::tests::test_atomic_wait_for_timeout_2() {
    // notify was called, value hasn't changed
    std::atomic_uint32_t flag {0};
    constexpr auto timeout_ms = 200;

    std::thread modifier([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms / 2));
        concurrencpp::details::atomic_notify_all(flag);
    });

    const auto before = std::chrono::high_resolution_clock::now();
    const auto result =
        concurrencpp::details::atomic_wait_for(flag, uint32_t(0), std::chrono::milliseconds(timeout_ms), std::memory_order_acquire);
    const auto after = std::chrono::high_resolution_clock::now();
    const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();

    assert_equal(result, concurrencpp::details::atomic_wait_status::timeout);
    assert_bigger_equal(time_diff, timeout_ms);
    assert_smaller_equal(time_diff, timeout_ms + 100);

    modifier.join();
}

void concurrencpp::tests::test_atomic_wait_for_success() {
    std::atomic_uint32_t flag {0};
    std::atomic<std::chrono::time_point<std::chrono::high_resolution_clock>> modification_tp { std::chrono::high_resolution_clock::now() };

    std::thread modifier([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        flag = 1;
        concurrencpp::details::atomic_notify_all(flag);
        modification_tp.store(std::chrono::high_resolution_clock::now());
    });

    const auto result = concurrencpp::details::atomic_wait_for(flag, uint32_t(0), std::chrono::seconds(10), std::memory_order_acquire);
    const auto after = std::chrono::high_resolution_clock::now();
    const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(after - modification_tp.load()).count();

    assert_equal(result, concurrencpp::details::atomic_wait_status::ok);
    assert_smaller_equal(time_diff, 210); // on github runners, thread awakening might even take up to 200 ms.. 

    modifier.join();
}

void concurrencpp::tests::test_atomic_wait_for() {
    test_atomic_wait_for_timeout_1();
    test_atomic_wait_for_timeout_2();
    test_atomic_wait_for_success();
}

void concurrencpp::tests::test_atomic_notify_one() {
    std::thread waiters[15];
    std::atomic_size_t woken = 0;
    std::atomic_uint32_t flag = 0;

    for (auto& waiter : waiters) {
        waiter = std::thread([&] {
            concurrencpp::details::atomic_wait(flag, uint32_t(0), std::memory_order_relaxed);
            woken.fetch_add(1, std::memory_order_acq_rel);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    flag = 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert_equal(woken.load(), 0);

    for (size_t i = 0; i < std::size(waiters); i++) {
        concurrencpp::details::atomic_notify_one(flag);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        assert_equal(woken.load(), i + 1);
    
    }

    for (auto& waiter : waiters) {
        waiter.join();
    }
}

void concurrencpp::tests::test_atomic_notify_all() {
    std::thread waiters[15];
    std::atomic_size_t woken = 0;
    std::atomic_uint32_t flag = 0;

    for (auto& waiter : waiters) {
        waiter = std::thread([&] {
            concurrencpp::details::atomic_wait(flag, uint32_t(0), std::memory_order_relaxed);
            woken.fetch_add(1, std::memory_order_acq_rel);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    flag = 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert_equal(woken.load(), 0);

    concurrencpp::details::atomic_notify_all(flag);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert_equal(woken.load(), std::size(waiters));

    for (auto& waiter : waiters) {
        waiter.join();
    }
}

void concurrencpp::tests::test_atomic_mini_load_test() {
    std::thread waiters[30];
    std::thread waiters_for[20];
    std::thread wakers[20];
    std::atomic_uint32_t atom {0};

    const auto test_timeout = std::chrono::system_clock::now() + std::chrono::seconds(20);

    for (auto& waiter : waiters) {
        waiter = std::thread([&] {
            while (std::chrono::system_clock::now() < test_timeout) {
                concurrencpp::details::atomic_wait(atom, uint32_t(0), std::memory_order_acquire);
            }
        });
    }

    for (auto& waiter_for : waiters_for) {
        waiter_for = std::thread([&] {
            while (std::chrono::system_clock::now() < test_timeout) {
                concurrencpp::details::atomic_wait_for(atom, uint32_t(0), std::chrono::milliseconds(2), std::memory_order_acquire);
            }
        });
    }

    for (auto& waker : wakers) {
        waker = std::thread([&] {
            int counter = 0;
            while (std::chrono::system_clock::now() < test_timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));

                if (counter % 2 == 0) {
                    atom.fetch_add(1, std::memory_order_acq_rel);
                } else {
                    atom.store(0, std::memory_order_release);
                }
                
                if (counter % 3 == 0) {
                    concurrencpp::details::atomic_notify_all(atom);
                } else {
                    concurrencpp::details::atomic_notify_one(atom);
                }
                
                counter++;
            }
        });
    }

    for (auto& waker : wakers) {
        waker.join();
    }

    atom.store(1, std::memory_order_release);
    concurrencpp::details::atomic_notify_all(atom);

    for (auto& waiter_for : waiters_for) {
        waiter_for.join();
    }

    for (auto& waiter : waiters) {
        waiter.join();
    }
}

int main() {
    tester tester("atomic_wait test");

    tester.add_step("wait", test_atomic_wait);
    tester.add_step("wait_for", test_atomic_wait_for);
    tester.add_step("notify_one", test_atomic_notify_one);
    tester.add_step("notify_all", test_atomic_notify_all);
    tester.add_step("mini load test", test_atomic_mini_load_test);

    tester.launch_test();
    return 0;
}
