#ifndef CONCURRENCPP_ATOMIC_WAIT_H
#define CONCURRENCPP_ATOMIC_WAIT_H

#include "concurrencpp/platform_defs.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <type_traits>

#include <cassert>

namespace concurrencpp::details {
    void CRCPP_API atomic_wait_native(void* atom, uint32_t old, std::memory_order order) noexcept;
    void CRCPP_API atomic_wait_for_native(void* atom, uint32_t old, std::chrono::milliseconds ms, std::memory_order order) noexcept;
    void CRCPP_API atomic_notify_all_native(void* atom) noexcept;

    enum class atomic_wait_status { ok, timeout };

    template<class type>
    void atomic_wait(std::atomic<type>& atom, type old, std::memory_order order) noexcept {
        static_assert(std::is_standard_layout_v<std::atomic<type>>, "atomic_wait - std::atom<type> is not standard-layout");
        static_assert(std::atomic<type>::is_always_lock_free, "atomic_wait - std::atom<type> is not lock free");
        static_assert(sizeof(type) == sizeof(uint32_t), "atomic_wait - <<type>> must be 4 bytes.");


        while (true) {
            const auto val = atom.load(order);
            if (val != old) {
                return;
            }

            atomic_wait_native(&atom, static_cast<uint32_t>(old), order);
        }
    }

    template<class type>
    atomic_wait_status atomic_wait_for(std::atomic<type>& atom,
                                       type old,
                                       std::chrono::milliseconds ms,
                                       std::memory_order order) noexcept {
        static_assert(std::is_standard_layout_v<std::atomic<type>>, "atomic_wait - std::atom<type> is not standard-layout");
        static_assert(std::atomic<type>::is_always_lock_free, "atomic_wait - std::atom<type> is not lock free");
        static_assert(sizeof(type) == sizeof(uint32_t), "atomic_wait - <<type>> must be 4 bytes.");

        const auto deadline = std::chrono::system_clock::now() + ms;

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

            const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            assert(time_diff.count() >= 0);
            atomic_wait_for_native(&atom, static_cast<uint32_t>(old), time_diff, order);
        }
    }

    template<class type>
    void atomic_notify_all(std::atomic<type>& atom) noexcept {
        atomic_notify_all_native(&atom);
    }
}  // namespace concurrencpp::details

#endif
