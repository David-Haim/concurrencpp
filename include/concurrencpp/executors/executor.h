#ifndef CONCURRENCPP_EXECUTOR_H
#define CONCURRENCPP_EXECUTOR_H

#include "concurrencpp/task.h"
#include "concurrencpp/results/result.h"

#include <span>
#include <vector>
#include <string>
#include <string_view>

namespace concurrencpp::details {
    [[noreturn]] void throw_runtime_shutdown_exception(std::string_view executor_name);
    std::string make_executor_worker_name(std::string_view executor_name);
}  // namespace concurrencpp::details

namespace concurrencpp {
    class executor {

       private:
        template<class return_type, class executor_type, class callable_type, class... argument_types>
        static result<return_type> submit_bridge(executor_tag, executor_type&, callable_type callable, argument_types... arguments) {
            co_return callable(arguments...);
        }

        template<class callable_type, class return_type = typename std::invoke_result_t<callable_type>>
        static result<return_type> bulk_submit_bridge(details::executor_bulk_tag,
                                                      std::vector<concurrencpp::task>& accumulator,
                                                      callable_type callable) {
            co_return callable();
        }

       protected:
        template<class executor_type, class callable_type, class... argument_types>
        static void do_post(executor_type& executor_ref, callable_type&& callable, argument_types&&... arguments) {
            static_assert(std::is_invocable_v<callable_type, argument_types...>,
                          "concurrencpp::executor::post - <<callable_type>> is not invokable with <<argument_types...>>");

            executor_ref.enqueue(
                details::bind_with_try_catch(std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...));
        }

        template<class executor_type, class callable_type, class... argument_types>
        static auto do_submit(executor_type& executor_ref, callable_type&& callable, argument_types&&... arguments) {
            static_assert(std::is_invocable_v<callable_type, argument_types...>,
                          "concurrencpp::executor::submit - <<callable_type>> is not invokable with <<argument_types...>>");

            using return_type = typename std::invoke_result_t<callable_type, argument_types...>;
            return submit_bridge<return_type>({},
                                              executor_ref,
                                              std::forward<callable_type>(callable),
                                              std::forward<argument_types>(arguments)...);
        }

        template<class executor_type, class callable_type>
        static void do_bulk_post(executor_type& executor_ref, std::span<callable_type> callable_list) {
            assert(!callable_list.empty());

            std::vector<task> tasks;
            tasks.reserve(callable_list.size());

            for (auto& callable : callable_list) {
                tasks.emplace_back(details::bind_with_try_catch(std::move(callable)));
            }

            std::span<task> span = tasks;
            executor_ref.enqueue(span);
        }

        template<class executor_type, class callable_type, class return_type = std::invoke_result_t<callable_type>>
        static std::vector<concurrencpp::result<return_type>> do_bulk_submit(executor_type& executor_ref,
                                                                             std::span<callable_type> callable_list) {
            std::vector<task> accumulator;
            accumulator.reserve(callable_list.size());

            std::vector<result<return_type>> results;
            results.reserve(callable_list.size());

            for (auto& callable : callable_list) {
                results.emplace_back(bulk_submit_bridge<callable_type>({}, accumulator, std::move(callable)));
            }

            assert(!accumulator.empty());
            std::span<task> span = accumulator;
            executor_ref.enqueue(span);
            return results;
        }

       public:
        executor(std::string_view name) : name(name) {}

        virtual ~executor() noexcept = default;

        const std::string name;

        virtual void enqueue(concurrencpp::task task) = 0;
        virtual void enqueue(std::span<concurrencpp::task> tasks) = 0;

        virtual int max_concurrency_level() const noexcept = 0;

        virtual bool shutdown_requested() const = 0;
        virtual void shutdown() = 0;

        template<class callable_type, class... argument_types>
        void post(callable_type&& callable, argument_types&&... arguments) {
            return do_post(*this, std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
        }

        template<class callable_type, class... argument_types>
        auto submit(callable_type&& callable, argument_types&&... arguments) {
            return do_submit(*this, std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
        }

        template<class callable_type>
        void bulk_post(std::span<callable_type> callable_list) {
            return do_bulk_post(*this, callable_list);
        }

        template<class callable_type, class return_type = std::invoke_result_t<callable_type>>
        std::vector<concurrencpp::result<return_type>> bulk_submit(std::span<callable_type> callable_list) {
            return do_bulk_submit(*this, callable_list);
        }
    };
}  // namespace concurrencpp

#endif
