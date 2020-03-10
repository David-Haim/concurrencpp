#include "concurrencpp.h"
#include "all_tests.h"

#include "../tester/tester.h"
#include "../helpers/assertions.h"

#include <thread>
#include <mutex>
#include <array>
#include <chrono>

using concurrencpp::details::spinlock;

namespace concurrencpp::tests {
	void test_spin_lock_lock_unlock_test_0();
	void test_spin_lock_lock_unlock_test_1();
	void test_spin_lock_lock_unlock_test_2();
	void test_spin_lock_try_lock();
}

void concurrencpp::tests::test_spin_lock_lock_unlock_test_0() {
	std::array<std::thread, 24> threads;
	spinlock lock;
	std::atomic_size_t counter = 0;

	const auto time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(4);
	auto cb = [&lock, &counter, time_point] {
		std::this_thread::sleep_until(time_point);

		for (size_t i = 0; i < 1024 * 100; i++) {
			std::unique_lock<spinlock> scoped_lock(lock);
			const auto num_of_threads_inside_critical_section = counter.fetch_add(1, std::memory_order_acq_rel);
			assert_same(num_of_threads_inside_critical_section, 0);
			counter.store(0, std::memory_order_release);
		}
	};

	for (auto& thread : threads) {
		thread = std::thread(cb);
	}

	for (auto& thread : threads) {
		thread.join();
	}
}

void concurrencpp::tests::test_spin_lock_lock_unlock_test_1() {
	std::array<std::thread, 24> threads;
	spinlock lock;
	size_t counter = 0;

	const auto time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(4);

	for (auto& thread : threads) {
		thread = std::thread([&lock, &counter, time_point] {
			std::this_thread::sleep_until(time_point);
			std::unique_lock<spinlock> scoped_lock(lock);
			++counter;
		});
	}

	for (auto& thread : threads) {
		thread.join();
	}

	std::unique_lock<spinlock> scoped_lock(lock);
	assert_same(counter, threads.size());
}

void concurrencpp::tests::test_spin_lock_lock_unlock_test_2() {
	std::array<std::thread, 16> threads;
	spinlock lock;
	std::atomic_bool locked = false;

	std::unique_lock<spinlock> scoped_lock(lock);

	const auto time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(4);

	for (auto& thread : threads) {
		thread = std::thread([&lock, &locked, time_point] {
			std::this_thread::sleep_until(time_point);
			std::unique_lock<spinlock> scoped_lock(lock);
			locked = true;
		});
	}

	for (size_t i = 0; i < 1024; i++) {
		assert_false(locked.load(std::memory_order_acquire));
		std::this_thread::yield();
	}

	scoped_lock.unlock();
	for (auto& thread : threads) {
		thread.join();
	}

	assert_true(locked.load(std::memory_order_acquire));
}

void concurrencpp::tests::test_spin_lock_try_lock() {
	//basic test:
	{
		spinlock lock;
		assert_true(lock.try_lock());
	}

	//if lock is taken, other threads fail acquiring the lock
	{
		std::array<std::thread, 24> threads;
		spinlock lock;
		std::atomic_size_t lockers_count = 0;

		std::unique_lock<spinlock> scoped_lock(lock);

		const auto time_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(4);

		for (auto& thread : threads) {
			thread = std::thread([&lock, &lockers_count, time_point] {
				std::this_thread::sleep_until(time_point);

				for (auto i = 0; i < 1024; i++) {
					const auto locked = lock.try_lock();
					lockers_count.fetch_add(locked ? 1 : 0, std::memory_order_acq_rel);
				}
			});
		}

		for (auto& thread : threads) {
			thread.join();
		}

		assert_same(lockers_count.load(), 0);
	}

	//when lock is released, it is free to be reacquired
	{
		spinlock lock;
		std::atomic_bool locked = false;

		std::unique_lock<spinlock> scoped_lock(lock);
		std::thread thread([&lock, &locked] {
			locked = lock.try_lock();
		});
		thread.join();

		assert_false(locked);
		scoped_lock.unlock();

		thread = std::thread([&lock, &locked] {
			locked = lock.try_lock();
		});
		thread.join();

		assert_true(locked);
	}
}

void concurrencpp::tests::test_spin_lock() {
	tester tester("spinlock test");

	tester.add_step("lock/unlock subtest0", test_spin_lock_lock_unlock_test_0);
	tester.add_step("lock/unlock subtest1", test_spin_lock_lock_unlock_test_1);
	tester.add_step("lock/unlock subtest2", test_spin_lock_lock_unlock_test_2);
	tester.add_step("try_lock", test_spin_lock_try_lock);

	tester.launch_test();
}