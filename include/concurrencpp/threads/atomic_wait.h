#ifndef CONCURRENCPP_ATOMIC_WAIT_H
#define CONCURRENCPP_ATOMIC_WAIT_H

#include "concurrencpp/platform_defs.h"

#include <atomic>
#include <chrono>
#include <type_traits>

#include <cassert>

namespace concurrencpp::details {

    enum class atomic_wait_status { ok, timeout };

    template<class type>
    void atomic_wait(std::atomic<type>& atom, type old, std::memory_order order) noexcept;

    template<class type>
    atomic_wait_status atomic_wait_for(std::atomic<type>& atom,
                                       type old,
                                       std::chrono::milliseconds ms,
                                       std::memory_order order) noexcept;

    template<class type>
    void atomic_notify_one(std::atomic<type>& atom) noexcept;

    template<class type>
    void atomic_notify_all(std::atomic<type>& atom) noexcept;

    template<class type>
    void assert_atomic_type_waitable() noexcept {
        static_assert(std::is_integral_v<type> || std::is_enum_v<type>,
                      "atomic_wait/atomic_notify - <<type>> must be integeral or enumeration type");
        static_assert(sizeof(type) == sizeof(uint32_t), "atomic_wait/atomic_notify - <<type>> must be 4 bytes.");
        static_assert(std::is_standard_layout_v<std::atomic<type>>,
                      "atomic_wait/atomic_notify - std::atom<type> is not standard-layout");
        static_assert(std::atomic<type>::is_always_lock_free, "atomic_wait/atomic_notify - std::atom<type> is not lock free");
    }
}  // namespace concurrencpp::details

#if !defined(CRCPP_MAC_OS)

namespace concurrencpp::details {
    void CRCPP_API atomic_wait_native(void* atom, uint32_t old) noexcept;
    void CRCPP_API atomic_wait_for_native(void* atom, uint32_t old, std::chrono::milliseconds ms) noexcept;
    void CRCPP_API atomic_notify_one_native(void* atom) noexcept;
    void CRCPP_API atomic_notify_all_native(void* atom) noexcept;

    template<class type>
    void atomic_wait(std::atomic<type>& atom, type old, std::memory_order order) noexcept {
        assert_atomic_type_waitable<type>();

        while (true) {
            const auto val = atom.load(order);
            if (val != old) {
                return;
            }

            atomic_wait_native(&atom, static_cast<uint32_t>(old));
        }
    }

    template<class type>
    atomic_wait_status atomic_wait_for(std::atomic<type>& atom,
                                       type old,
                                       std::chrono::milliseconds ms,
                                       std::memory_order order) noexcept {
        assert_atomic_type_waitable<type>();

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
            atomic_wait_for_native(&atom, static_cast<uint32_t>(old), time_diff);
        }
    }

    template<class type>
    void atomic_notify_one(std::atomic<type>& atom) noexcept {
        atomic_notify_one_native(&atom);
    }

    template<class type>
    void atomic_notify_all(std::atomic<type>& atom) noexcept {
        atomic_notify_all_native(&atom);
    }
}  // namespace concurrencpp::details

#else

#include <memory>

namespace concurrencpp::details {
    class atomic_wait_bucket;

    using atomic_comp_fn = bool (*)(void*, const uint32_t, std::memory_order) noexcept;

    class CRCPP_API atomic_wait_table {

       private:
        std::unique_ptr<atomic_wait_bucket[]> m_buckets;
        const size_t m_size;

        static size_t calc_table_size() noexcept;
        size_t index_for(const void* atom) const noexcept;

       public:
        atomic_wait_table();

        void wait(void* atom, const uint32_t old, std::memory_order order, atomic_comp_fn comp);
        atomic_wait_status wait_for(void* atom,
                                    const uint32_t old,
                                    std::chrono::milliseconds ms,
                                    std::memory_order order,
                                    atomic_comp_fn comp);

        void notify_one(const void* atom) noexcept;
        void notify_all(const void* atom) noexcept;

        static atomic_wait_table& instance();
    };

    template<class type>
    void atomic_wait(std::atomic<type>& atom, type old, std::memory_order order) noexcept {
        assert_atomic_type_waitable<type>();

        auto comp = [](void* atom_, const uint32_t old_, std::memory_order order_) noexcept -> bool {
            auto& original_atom = *static_cast<std::atomic<type>*>(atom_);
            const auto original_old = static_cast<type>(old_);

            return original_atom.load(order_) == original_old;
        };

        atomic_wait_table::instance().wait(&atom, static_cast<uint32_t>(old), order, comp);
    }

    template<class type>
    atomic_wait_status atomic_wait_for(std::atomic<type>& atom,
                                       type old,
                                       std::chrono::milliseconds ms,
                                       std::memory_order order) noexcept {
        assert_atomic_type_waitable<type>();

        auto comp = [](void* atom_, const uint32_t old_, std::memory_order order_) noexcept -> bool {
            auto& original_atom = *static_cast<std::atomic<type>*>(atom_);
            const auto original_old = static_cast<type>(old_);

            return original_atom.load(order_) == original_old;
        };

        return atomic_wait_table::instance().wait_for(&atom, static_cast<uint32_t>(old), ms, order, comp);
    }

    template<class type>
    void atomic_notify_one(std::atomic<type>& atom) noexcept {
        atomic_wait_table::instance().notify_one(&atom);
    }

    template<class type>
    void atomic_notify_all(std::atomic<type>& atom) noexcept {
        atomic_wait_table::instance().notify_all(&atom);
    }

}  // namespace concurrencpp::details

#endif

#endif
