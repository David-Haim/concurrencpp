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
    int futex(void* addr, int32_t op, int32_t old, const timespec* ts) noexcept {
        return ::syscall(SYS_futex, addr, op, old, ts, nullptr, 0);
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

    void atomic_notify_all_native(void* atom) noexcept {
        futex(atom, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr);
    }
}  // namespace concurrencpp::details

#else

#    include <mutex>
#    include <thread>
#    include <memory>
#    include <condition_variable>

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

        void set_notified(std::unique_lock<std::mutex>& lock) noexcept {
            assert(lock.owns_lock());
            m_notified = true;
        }

        void notify_one(std::unique_lock<std::mutex>& lock) noexcept {
            assert(lock.owns_lock());
            set_notified(lock);

            m_cv.notify_one();
        }

        void notify_all(std::unique_lock<std::mutex>& lock) noexcept {
            assert(lock.owns_lock());
            set_notified(lock);

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
        waiting_node* m_head = nullptr;

#    if defined(CRCPP_DEBUG_MODE)
        size_t size = 0;
#    endif

        void insert_node(std::unique_lock<std::mutex>& lock, waiting_node& new_node) noexcept {
            assert(lock.owns_lock());
            assert(new_node.next == nullptr);
            assert(new_node.prev == nullptr);
            assert(new_node.address() != nullptr);

#    if defined(CRCPP_DEBUG_MODE)
            ++size;
#    endif

            if (m_head == nullptr) {
                m_head = &new_node;
                return;
            }

            new_node.next = m_head;

            assert(m_head->prev == nullptr);
            m_head->prev = &new_node;
            m_head = &new_node;
        }

        void remove_node(std::unique_lock<std::mutex>& lock, waiting_node& old_node) noexcept {
            assert(lock.owns_lock());

#    if defined(CRCPP_DEBUG_MODE)
            assert(size != 0);
            --size;
#    endif

            if (&old_node == m_head) {
                m_head = m_head->next;

                if (m_head != nullptr) {
                    m_head->prev = nullptr;
                }

                return;
            }

            auto prev_node = old_node.prev;
            assert(prev_node != nullptr);

            auto next_node = old_node.next;
            prev_node->next = next_node;

            if (next_node != nullptr) {
                next_node->prev = prev_node;
            }
        }

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
                insert_node(lock, node);
                node.wait(lock);

                assert(lock.owns_lock());
                remove_node(lock, node);
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
                insert_node(lock, node);
                node.wait_until(lock, later);

                assert(lock.owns_lock());
                remove_node(lock, node);
            }

            return atomic_wait_status::timeout;
        }

        void notify_one(const void* atom) noexcept {
            std::unique_lock<std::mutex> lock(m_lock);

            auto cursor = m_head;
            while (cursor != nullptr) {
                auto next = cursor->next;
                if (cursor->address() == atom) {
                    cursor->notify_one(lock);
                    return;
                }

                cursor = next;
            }
        }

        void notify_all(const void* atom) noexcept {
            std::unique_lock<std::mutex> lock(m_lock);

            auto cursor = m_head;
            while (cursor != nullptr) {
                auto next = cursor->next;
                if (cursor->address() == atom) {
                    cursor->notify_one(lock);
                }

                cursor = next;
            }
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

        auto is_prime = [](size_t n) noexcept {
            if (n <= 1) {
                return false;
            }

            if (n <= 3) {
                return true;
            }
            if (n % 2 == 0 || n % 3 == 0) {
                return false;
            }

            for (size_t i = 5; i * i <= n; i += 6) {
                if (n % i == 0 || n % (i + 2) == 0) {
                    return false;
                }
            }

            return true;
        };

        auto next_prime = [is_prime](size_t n) noexcept -> size_t {
            if (n <= 1) {
                return 2;
            }

            size_t prime = n;

            while (!is_prime(prime)) {
                prime++;
            }

            return prime;
        };

        const auto padded_hc = hc * 2;
        return next_prime(padded_hc);
    }

    size_t atomic_wait_table::index_for(const void* atom) const noexcept {
        return std::hash<const void*>()(atom) % m_size;
    }

    atomic_wait_table::atomic_wait_table() : m_size(calc_table_size()) {
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
