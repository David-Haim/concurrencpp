#ifndef CONCURRENCPP_SPINLOCK_H
#define CONCURRENCPP_SPINLOCK_H

#include "../platform_defs.h"

#include <atomic>
#include <thread>
#include <chrono>

#ifdef CRCPP_MSVC_COMPILER

extern "C" void _mm_pause();

#endif

namespace concurrencpp::details {
	class spinlock {

		constexpr static size_t k_spin_count = 128;
		constexpr static std::chrono::milliseconds k_sleep_duration = std::chrono::milliseconds(1);

	private:
		std::atomic_flag m_flag = ATOMIC_FLAG_INIT;

		static void pause_cpu() noexcept {
#ifdef CRCPP_MSVC_COMPILER
			_mm_pause();
#elif defined(CRCPP_GCC_COMPILER) || defined(CRCPP_CLANG_COMPILER)
			asm volatile("pause\n": : : "memory");
#endif
		}

		static void spin(size_t current_spincount) noexcept {
			if (current_spincount < k_spin_count) { return pause_cpu(); }
			if (current_spincount < k_spin_count * 2) { return std::this_thread::yield(); }

			std::this_thread::sleep_for(k_sleep_duration);
		}

	public:
		spinlock() noexcept = default;
		spinlock(spinlock&&) noexcept = delete;
		spinlock(const spinlock&) noexcept = delete;

		void unlock() noexcept { m_flag.clear(); }
		bool try_lock() noexcept { return m_flag.test_and_set(std::memory_order_acquire) == false; }

		void lock() noexcept {
			size_t counter = 0;
			while (!try_lock()) {
				spin(counter);
				++counter;
			}
		}
	};
}

#endif