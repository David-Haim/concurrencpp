#include "concurrencpp/platform_defs.h"
#include "concurrencpp/threads/atomic_wait.h"

#include <cassert>

#if defined(CRCPP_WIN_OS)

#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    pragma comment(lib, "Synchronization.lib")

namespace concurrencpp::details {
    void atomic_wait_native(void* atom, int32_t old) noexcept {
        ::WaitOnAddress(atom, &old, sizeof(old), INFINITE);
    }

    void atomic_wait_for_native(void* atom, int32_t old, std::chrono::milliseconds ms) noexcept {
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

    void atomic_wait_native(void* atom, int32_t old) noexcept {
        futex(atom, FUTEX_WAIT_PRIVATE, old, nullptr);
    }

    void atomic_wait_for_native(void* atom, int32_t old, std::chrono::milliseconds ms) noexcept {
        auto spec = ms_to_time_spec(ms.count());
        futex(atom, FUTEX_WAIT_PRIVATE, old, &spec);
    }

    void atomic_notify_all_native(void* atom) noexcept {
        futex(atom, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr);
    }
}  // namespace concurrencpp::details

#else

namespace concurrencpp::details {
    void atomic_wait_native(void* atom, int32_t old) noexcept {}

    void atomic_wait_for_native(void* atom, int32_t old, std::chrono::milliseconds ms, size_t* polling_cycle_ptr) noexcept {}

    void atomic_notify_all_native(void* atom) noexcept {}
}  // namespace concurrencpp::details

#endif