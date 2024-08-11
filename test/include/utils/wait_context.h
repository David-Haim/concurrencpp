#ifndef CONCURRENCPP_WAIT_CONTEXT_H
#define CONCURRENCPP_WAIT_CONTEXT_H

#include "concurrencpp/threads/atomic_wait.h"

namespace concurrencpp::tests {
    class wait_context {

       private:
        std::atomic_uint32_t m_ready {0};

       public:
        void wait();
        bool wait_for(size_t milliseconds);

        void notify();
    };
}  // namespace concurrencpp::tests

#endif