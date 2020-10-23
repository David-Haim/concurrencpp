#ifndef MOCK_PROCESS_MONITOR_H
#define MOCK_PROCESS_MONITOR_H

#include <cstddef>

namespace mock_process_monitor {
    class monitor {

       private:
        size_t m_last_cpu_usage;
        size_t m_last_memory_usage;
        size_t m_last_thread_count;
        size_t m_last_kernel_object_count;

       public:
        monitor() noexcept;

        size_t cpu_usage() noexcept;
        size_t memory_usage() noexcept;
        size_t thread_count() noexcept;
        size_t kernel_object_count() noexcept;
    };
}  // namespace mock_process_monitor

#endif
