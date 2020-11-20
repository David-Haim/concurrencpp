#ifndef CONCURRENCPP_THREAD_H
#define CONCURRENCPP_THREAD_H

#include <string_view>
#include <thread>

namespace concurrencpp::details {
    class thread {

       private:
        std::thread m_thread;

        static void set_name(std::string_view name) noexcept;

       public:
        thread() noexcept = default;
        thread(thread&&) noexcept = default;

        template<class callable_type>
        thread(std::string name, callable_type&& callable) {
            m_thread = std::thread([name = std::move(name), callable = std::forward<callable_type>(callable)]() mutable {
                set_name(name);
                callable();
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
