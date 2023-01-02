#ifndef CONCURRENCPP_DERIVABLE_EXECUTOR_H
#define CONCURRENCPP_DERIVABLE_EXECUTOR_H

#include "concurrencpp/utils/bind.h"
#include "concurrencpp/executors/executor.h"

namespace concurrencpp {
    template<class concrete_executor_type>
    struct CRCPP_API derivable_executor : public executor {

        derivable_executor(std::string_view name) : executor(name) {}

        template<class callable_type, class... argument_types>
        void post(callable_type&& callable, argument_types&&... arguments) {
            return do_post<concrete_executor_type>(std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
        }

        template<class callable_type, class... argument_types>
        auto submit(callable_type&& callable, argument_types&&... arguments) {
            return do_submit<concrete_executor_type>(std::forward<callable_type>(callable),
                                                     std::forward<argument_types>(arguments)...);
        }

        template<class callable_type>
        void bulk_post(std::span<callable_type> callable_list) {
            return do_bulk_post<concrete_executor_type>(callable_list);
        }

        template<class callable_type, class return_type = std::invoke_result_t<callable_type>>
        std::vector<concurrencpp::result<return_type>> bulk_submit(std::span<callable_type> callable_list) {
            return do_bulk_submit<concrete_executor_type>(callable_list);
        }
    };
}  // namespace concurrencpp

#endif
