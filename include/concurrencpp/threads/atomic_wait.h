#ifndef CONCURRENCPP_ATOMIC_WAIT_H
#define CONCURRENCPP_ATOMIC_WAIT_H

#include "concurrencpp/platform_defs.h"

#include <thread>
#include <atomic>
#include <chrono>

#include <cassert>

namespace concurrencpp::details {
    void CRCPP_API atomic_wait_native(void* atom, int32_t old) noexcept;
    void CRCPP_API atomic_wait_for_native(void* atom, int32_t old, std::chrono::milliseconds ms) noexcept;
    void CRCPP_API atomic_notify_all_native(void* atom) noexcept;

    enum class atomic_wait_status { ok, timeout };

    template<class type>
    void atomic_wait(std::atomic<type>& atom, type old, std::memory_order order) noexcept {
        static_assert(std::is_standard_layout_v<std::atomic<type>>, "atomic_wait - std::atom<type> is not standard-layout");
        static_assert(sizeof(type) == sizeof(int32_t), "atomic_wait - <<type>> must be 4 bytes.");

        while (true) {
            const auto val = atom.load(order);
            if (val != old) {
                return;
            }

#if defined(CRCPP_MAC_OS)
            atom.wait(old, order);
#else
            atomic_wait_native(&atom, static_cast<int32_t>(old));
#endif
        }
    }

    template<class type>
    atomic_wait_status atomic_wait_for(std::atomic<type>& atom,
                                       type old,
                                       std::chrono::milliseconds ms,
                                       std::memory_order order) noexcept {
        static_assert(std::is_standard_layout_v<std::atomic<type>>, "atomic_wait_for - std::atom<type> is not standard-layout");
        static_assert(sizeof(type) == sizeof(int32_t), "atomic_wait_for - <<type>> must be 4 bytes.");

        const auto deadline = std::chrono::system_clock::now() + ms;

#if defined(CRCPP_MAC_OS)
        size_t polling_cycle = 0;
#endif
        while (true) {
            if (atom.load(order) != old) {
                return atomic_wait_status::ok;
            }

            const auto now = std::chrono::system_clock::now();
            if (now >= deadline) {
                if (atom.load(order) != old) {
                    return atomic_wait_status::ok;
                }

                return atomic_wait_status::timeout;
            }

#if defined(CRCPP_MAC_OS)
            if (polling_cycle < 64) {
                std::this_thread::yield();
                ++polling_cycle;
                continue;
            }

            if (polling_cycle < 5'000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ++polling_cycle;
                continue;
            }

            if (polling_cycle < 10'000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                ++polling_cycle;
                continue;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            ++polling_cycle;

#else
            const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            assert(time_diff.count() >= 0);
            atomic_wait_for_native(&atom, static_cast<int32_t>(old), time_diff);
#endif
        }
    }

    template<class type>
    void atomic_notify_all(std::atomic<type>& atom) noexcept {
#if defined(CRCPP_MAC_OS)

        atom.notify_all();
#else
        atomic_notify_all_native(&atom);
#endif
    }
}  // namespace concurrencpp::details

#endif
