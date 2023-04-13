#ifndef CONCURRENCPP_DERIVABLE_EXECUTOR_H
#define CONCURRENCPP_DERIVABLE_EXECUTOR_H

#include "concurrencpp/executors/executor.h"

namespace concurrencpp {
    template<class concrete_executor_type>
    struct CRCPP_API derivable_executor : public executor {

        derivable_executor(std::string_view name) : executor(name) {}
    };
}  // namespace concurrencpp

#endif
