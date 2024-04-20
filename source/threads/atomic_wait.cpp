#include "concurrencpp/platform_defs.h"
#include "concurrencpp/threads/atomic_wait.h"

#include <cassert>

#if defined(CRCPP_WIN_OS)

#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    pragma comment(lib, "Synchronization.lib")

namespace concurrencpp::details {
    void atomic_wait_native(void* atom, uint32_t old) noexcept {
        ::WaitOnAddress(atom, &old, sizeof(old), INFINITE);
    }

    void atomic_wait_for_native(void* atom, uint32_t old, std::chrono::milliseconds ms) noexcept {
        ::WaitOnAddress(atom, &old, sizeof(old), static_cast<DWORD>(ms.count()));
    }

    void atomic_notify_one_native(void* atom) noexcept {
        ::WakeByAddressSingle(atom);
    }

    void atomic_notify_all_native(void* atom) noexcept {
        ::WakeByAddressAll(atom);
    }
}  // namespace concurrencpp::details

#elif defined(CRCPP_UNIX_OS) || defined(CRCPP_FREE_BSD_OS)

#    include <ctime>

#    include <unistd.h>
#    include <linux/futex.h>
#    include <sys/syscall.h>

namespace concurrencpp::details {
    int futex(void* addr, int32_t op, uint32_t val, const timespec* ts) noexcept {
        return ::syscall(SYS_futex, addr, op, val, ts, nullptr, 0);
    }

    timespec ms_to_time_spec(size_t ms) noexcept {
        timespec req;
        req.tv_sec = static_cast<time_t>(ms / 1000);
        req.tv_nsec = (ms % 1000) * 1'000'000;
        return req;
    }

    void atomic_wait_native(void* atom, uint32_t old) noexcept {
        futex(atom, FUTEX_WAIT_PRIVATE, old, nullptr);
    }

    void atomic_wait_for_native(void* atom, uint32_t old, std::chrono::milliseconds ms) noexcept {
        auto spec = ms_to_time_spec(ms.count());
        futex(atom, FUTEX_WAIT_PRIVATE, old, &spec);
    }

    void atomic_notify_one_native(void* atom) noexcept {
        futex(atom, FUTEX_WAKE_PRIVATE, 1, nullptr);
    }

    void atomic_notify_all_native(void* atom) noexcept {
        futex(atom, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr);
    }
}  // namespace concurrencpp::details

#else

#    include <mutex>
#    include <thread>
#    include <memory>
#    include <condition_variable>

#    include "concurrencpp/utils/dlist.h"
#    include "concurrencpp/utils/math_helper.h"

namespace concurrencpp::details {

    struct waiting_node {
        waiting_node* next = nullptr;
        waiting_node* prev = nullptr;

       private:
        const void* const m_address;
        std::condition_variable m_cv;
        bool m_notified = false;

       public:
        waiting_node(const void* address) noexcept : m_address(address) {}

        void notify_one(std::unique_lock<std::mutex>& lock) noexcept {
            assert(lock.owns_lock());
            m_notified = true;

            m_cv.notify_one();
        }

        void wait(std::unique_lock<std::mutex>& lock) {
            assert(lock.owns_lock());

            m_cv.wait(lock, [this] {
                return m_notified;
            });

            assert(m_notified);
        }

        void wait_until(std::unique_lock<std::mutex>& lock, std::chrono::system_clock::time_point tp) {
            assert(lock.owns_lock());

            m_cv.wait_until(lock, tp, [this] {
                return m_notified;
            });
        }

        const void* address() const noexcept {
            return m_address;
        }
    };

    class atomic_wait_bucket {

       private:
        std::mutex m_lock;
        dlist<waiting_node> m_nodes;

       public:
        void wait(void* atom, const uint32_t old, std::memory_order order, atomic_comp_fn comp) {
            while (true) {
                if (!comp(atom, old, order)) {
                    return;
                }

                std::unique_lock<std::mutex> lock(m_lock);
                if (!comp(atom, old, order)) {
                    return;
                }

                waiting_node node(atom);
                m_nodes.push_front(node);
                node.wait(lock);

                assert(lock.owns_lock());
                m_nodes.remove_node(node);
            }
        }

        atomic_wait_status wait_for(void* atom,
                                    const uint32_t old,
                                    std::chrono::milliseconds ms,
                                    std::memory_order order,
                                    atomic_comp_fn comp) {

            const auto later = std::chrono::system_clock::now() + ms;

            while (true) {
                if (!comp(atom, old, order)) {
                    return atomic_wait_status::ok;
                }

                if (std::chrono::system_clock::now() >= later) {
                    if (!comp(atom, old, order)) {
                        return atomic_wait_status::ok;
                    }

                    return atomic_wait_status::timeout;
                }

                std::unique_lock<std::mutex> lock(m_lock);
                if (!comp(atom, old, order)) {
                    return atomic_wait_status::ok;
                }

                waiting_node node(atom);
                m_nodes.push_front(node);
                node.wait_until(lock, later);

                assert(lock.owns_lock());
                m_nodes.remove_node(node);
            }

            return atomic_wait_status::timeout;
        }

        void notify_one(const void* atom) noexcept {
            std::unique_lock<std::mutex> lock(m_lock);

            m_nodes.for_each([&lock, atom](waiting_node& node) noexcept -> bool {
                if (node.address() == atom) {
                    node.notify_one(lock);
                    return false;
                }

                return true;
            });
        }

        void notify_all(const void* atom) noexcept {
            std::unique_lock<std::mutex> lock(m_lock);

            m_nodes.for_each([&lock, atom](waiting_node& node) noexcept -> bool {
                if (node.address() == atom) {
                    node.notify_one(lock);
                }

                return true;
            });
        }
    };

    /*
        atomic_wait_table
    */

    size_t atomic_wait_table::calc_table_size() noexcept {
        const auto hc = std::thread::hardware_concurrency();
        if (hc == 0) {
            return 37;  // heuristic. most modern CPUs have less than 64 cores, and 37 is a prime number
        }
        
        const auto padded_hc = hc * 2;
        return math_helper::next_prime(padded_hc);
    }

    size_t atomic_wait_table::index_for(const void* atom) const noexcept {
        return std::hash<const void*>()(atom) % m_size;
    }

    atomic_wait_table::atomic_wait_table() : m_size(calc_table_size()) {
        assert(m_size != 0);
        m_buckets = std::make_unique<atomic_wait_bucket[]>(m_size);
    }

    void atomic_wait_table::wait(void* atom, const uint32_t old, std::memory_order order, atomic_comp_fn comp) {
        const auto index = index_for(atom);
        m_buckets[index].wait(atom, old, order, comp);
    }

    atomic_wait_status atomic_wait_table::wait_for(void* atom,
                                                   const uint32_t old,
                                                   std::chrono::milliseconds ms,
                                                   std::memory_order order,
                                                   atomic_comp_fn comp) {

        const auto index = index_for(atom);
        return m_buckets[index].wait_for(atom, old, ms, order, comp);
    }

    void atomic_wait_table::notify_one(const void* atom) noexcept {
        const auto index = index_for(atom);
        m_buckets[index].notify_one(atom);
    }

    void atomic_wait_table::notify_all(const void* atom) noexcept {
        const auto index = index_for(atom);
        m_buckets[index].notify_all(atom);
    }

    atomic_wait_table& atomic_wait_table::instance() {
        static atomic_wait_table s_wait_table;
        return s_wait_table;
    }
}  // namespace concurrencpp::details

#endif
