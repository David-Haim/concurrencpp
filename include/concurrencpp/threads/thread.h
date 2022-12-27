#ifndef CONCURRENCPP_THREAD_H
#define CONCURRENCPP_THREAD_H

#include "concurrencpp/platform_defs.h"

#include <functional>
#include <string_view>
#include <thread>

namespace concurrencpp::details {
    class CRCPP_API thread {

       private:
        std::thread m_thread;

        static void set_name(std::string_view name) noexcept;

       public:
        thread() noexcept = default;
        thread(thread&&) noexcept = default;

        template<class callable_type>
        thread(std::string name,
               callable_type&& callable,
               std::function<void(const char* thread_name)> thread_started_callback,
               std::function<void(const char* thread_name)> thread_terminated_callback) {
            m_thread = std::thread([name = std::move(name),
                                    callable = std::forward<callable_type>(callable),
                                    thread_started_callback = std::move(thread_started_callback),
                                    thread_terminated_callback = std::move(thread_terminated_callback)]() mutable {
                set_name(name);

                if (thread_started_callback)
                    thread_started_callback(name.c_str());

                callable();

                if (thread_terminated_callback)
                    thread_terminated_callback(name.c_str());
            });
        }

        thread& operator=(thread&& rhs) noexcept = default;

        std::thread::id get_id() const noexcept;

        static std::uintptr_t get_current_virtual_id() noexcept;

        bool joinable() const noexcept;
        void join();

        static size_t hardware_concurrency() noexcept;
    };
}  // namespace concurrencpp::details

#endif
