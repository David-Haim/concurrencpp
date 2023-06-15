#include "concurrencpp/threads/thread.h"

#include "concurrencpp/platform_defs.h"

#include <atomic>

#include "concurrencpp/runtime/constants.h"

using concurrencpp::details::thread;

namespace concurrencpp::details {
    namespace {
        std::uintptr_t generate_thread_id() noexcept {
            static std::atomic_uintptr_t s_id_seed = 1;
            return s_id_seed.fetch_add(1, std::memory_order_relaxed);
        }

        struct thread_per_thread_data {
            const std::uintptr_t id = generate_thread_id();
        };

        thread_local thread_per_thread_data s_tl_thread_per_data;
    }  // namespace
}  // namespace concurrencpp::details

std::thread::id thread::get_id() const noexcept {
    return m_thread.get_id();
}

std::uintptr_t thread::get_current_virtual_id() noexcept {
    return s_tl_thread_per_data.id;
}

bool thread::joinable() const noexcept {
    return m_thread.joinable();
}

void thread::join() {
    m_thread.join();
}

size_t thread::hardware_concurrency() noexcept {
    const auto hc = std::thread::hardware_concurrency();
    return (hc != 0) ? hc : consts::k_default_number_of_cores;
}

#ifdef CRCPP_WIN_OS

#    include <Windows.h>

void thread::set_name(std::string_view name) noexcept {
    const std::wstring utf16_name(name.begin(),
                                  name.end());  // concurrencpp strings are always ASCII (english only)
    ::SetThreadDescription(::GetCurrentThread(), utf16_name.data());
}

#elif defined(CRCPP_MINGW_OS)

#    include <pthread.h>

void thread::set_name(std::string_view name) noexcept {
    ::pthread_setname_np(::pthread_self(), name.data());
}

#elif defined(CRCPP_UNIX_OS)

#    include <pthread.h>

void thread::set_name(std::string_view name) noexcept {
    ::pthread_setname_np(::pthread_self(), name.data());
}

#elif defined(CRCPP_MAC_OS)

#    include <pthread.h>

void thread::set_name(std::string_view name) noexcept {
    ::pthread_setname_np(name.data());
}

#endif
