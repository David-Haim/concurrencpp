#ifndef CONCURRENCPP_DERIVABLE_EXECUTOR_H
#define CONCURRENCPP_DERIVABLE_EXECUTOR_H

#include "concurrencpp/utils/bind.h"
#include "concurrencpp/executors/executor.h"

namespace concurrencpp {
    template<class concrete_executor_type>
    class derivable_executor : public executor {

       private:
        concrete_executor_type& self() noexcept {
            return *static_cast<concrete_executor_type*>(this);
        }

       public:
        derivable_executor(std::string_view name) : executor(name) {}

        template<class callable_type, class... argument_types>
        void post(callable_type&& callable, argument_types&&... arguments) {
            return do_post(self(), std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
        }

        template<class callable_type, class... argument_types>
        auto submit(callable_type&& callable, argument_types&&... arguments) {
            return do_submit(self(), std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
        }

        template<class callable_list_type>
        void bulk_post(callable_list_type &&callable_list) {
            using callable_type = std::remove_reference_t<decltype(*callable_list.data())>;
            return do_bulk_post(self(), std::span<callable_type>(callable_list.data(), callable_list.size()));
        }

        template<class callable_list_type>
        auto bulk_submit(callable_list_type &&callable_list) {
            using callable_type = std::remove_reference_t<decltype(*callable_list.data())>;
            return do_bulk_submit(self(), std::span<callable_type>(callable_list.data(), callable_list.size()));
        }
    };
}  // namespace concurrencpp

#endif
