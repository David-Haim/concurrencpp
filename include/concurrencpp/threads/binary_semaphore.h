#ifndef CONCURRENCPP_BINARY_SEMAPHORE_H
#define CONCURRENCPP_BINARY_SEMAPHORE_H

#include "concurrencpp/platform_defs.h"

#if defined(CRCPP_MAC_OS) && defined(CRCPP_LIBCPP_LIB)

#    include <mutex>
#    include <chrono>
#    include <condition_variable>

#    include <cstddef>

namespace concurrencpp::details {

    class CRCPP_API binary_semaphore {

       private:
        std::mutex m_lock;
        std::condition_variable m_condition;
        bool m_is_signaled;

        bool try_acquire_until_impl(const std::chrono::time_point<std::chrono::system_clock>& abs_time);

       public:
        binary_semaphore(std::ptrdiff_t desired);

        void release(std::ptrdiff_t update = 1);
        void acquire();
        bool try_acquire() noexcept;

        template<class Rep, class Period>
        bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time) {
            const auto deadline = std::chrono::system_clock::now() + rel_time;
            return try_acquire_until_impl(deadline);
        }

        template<class Clock, class Duration>
        bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            const auto src_now = Clock::now();
            const auto dst_now = std::chrono::system_clock::now();
            const auto deadline = dst_now + std::chrono::duration_cast<std::chrono::milliseconds>(abs_time - src_now);
            return try_acquire_until_impl(deadline);
        }
    };

}  // namespace concurrencpp::details

#else

#    include <semaphore>

namespace concurrencpp::details {
    using binary_semaphore = std::binary_semaphore;
}

#endif

#endif
