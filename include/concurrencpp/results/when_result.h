#ifndef CONCURRENCPP_WHEN_RESULT_H
#define CONCURRENCPP_WHEN_RESULT_H

#include "concurrencpp/errors.h"
#include "concurrencpp/results/resume_on.h"
#include "concurrencpp/results/lazy_result.h"

#include <tuple>
#include <memory>
#include <vector>

namespace concurrencpp::details {
    class when_result_helper {

       private:
        static void throw_if_empty_impl(const char* error_message) noexcept {
            (void)error_message;
        }

        template<class type, class... result_types>
        static void throw_if_empty_impl(const char* error_message, const result<type>& result, result_types&&... results) {
            if (!static_cast<bool>(result)) {
                throw errors::empty_result(error_message);
            }

            throw_if_empty_impl(error_message, std::forward<result_types>(results)...);
        }

        template<class type>
        static result_state_base& get_state_base(result<type>& result) noexcept {
            assert(static_cast<bool>(result.m_state));
            return *result.m_state;
        }

        template<std::size_t... is, typename tuple_type>
        static result_state_base& at_impl(std::index_sequence<is...>, tuple_type& tuple, size_t n) noexcept {
            result_state_base* bases[] = {(&get_state_base(std::get<is>(tuple)))...};
            assert(bases[n] != nullptr);
            return *bases[n];
        }

       public:
        template<typename tuple_type>
        static result_state_base& at(tuple_type& tuple, size_t n) noexcept {
            auto seq = std::make_index_sequence<std::tuple_size<tuple_type>::value>();
            return at_impl(seq, tuple, n);
        }

        template<typename type>
        static result_state_base& at(std::vector<result<type>>& vector, size_t n) noexcept {
            assert(n < vector.size());
            return get_state_base(vector[n]);
        }

        template<class... types>
        static size_t size(std::tuple<types...>& tuple) noexcept {
            return std::tuple_size_v<std::tuple<types...>>;
        }

        template<class type>
        static size_t size(const std::vector<type>& vector) noexcept {
            return vector.size();
        }

        template<class... result_types>
        static void throw_if_empty_tuple(const char* error_message, result_types&&... results) {
            throw_if_empty_impl(error_message, std::forward<result_types>(results)...);
        }

        template<class iterator_type>
        static void throw_if_empty_range(const char* error_message, iterator_type begin, iterator_type end) {
            for (; begin != end; ++begin) {
                if (!static_cast<bool>((*begin))) {
                    throw errors::empty_result(error_message);
                }
            }
        }

        class when_all_awaitable {

           private:
            result_state_base& m_state;

           public:
            when_all_awaitable(result_state_base& state) noexcept : m_state(state) {}

            bool await_ready() const noexcept {
                return false;
            }

            bool await_suspend(details::coroutine_handle<void> coro_handle) noexcept {
                return m_state.await(coro_handle);
            }

            void await_resume() const noexcept {}
        };

        template<class result_types>
        class when_any_awaitable {

           private:
            std::shared_ptr<when_any_context> m_promise;
            result_types& m_results;

           public:
            when_any_awaitable(result_types& results) noexcept : m_results(results) {}

            bool await_ready() const noexcept {
                return false;
            }

            bool await_suspend(coroutine_handle<void> coro_handle) {
                m_promise = std::make_shared<when_any_context>(coro_handle);

                const auto range_length = when_result_helper::size(m_results);
                for (size_t i = 0; i < range_length; i++) {
                    if (m_promise->any_result_finished()) {
                        return false;
                    }

                    auto& state_ref = when_result_helper::at(m_results, i);
                    const auto status = state_ref.when_any(m_promise);
                    if (status == result_state_base::pc_state::producer_done) {
                        return m_promise->resume_inline(state_ref);
                    }
                }

                return m_promise->finish_processing();
            }

            size_t await_resume() noexcept {
                const auto completed_result_state = m_promise->completed_result();
                auto completed_result_index = std::numeric_limits<size_t>::max();

                const auto range_length = when_result_helper::size(m_results);
                for (size_t i = 0; i < range_length; i++) {
                    auto& state_ref = when_result_helper::at(m_results, i);
                    state_ref.try_rewind_consumer();
                    if (completed_result_state == &state_ref) {
                        completed_result_index = i;
                    }
                }

                assert(completed_result_index != std::numeric_limits<size_t>::max());
                return completed_result_index;
            }
        };
    };
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class sequence_type>
    struct when_any_result {
        std::size_t index;
        sequence_type results;

        when_any_result() noexcept : index(static_cast<size_t>(-1)) {}

        template<class... result_types>
        when_any_result(size_t index, result_types&&... results) noexcept :
            index(index), results(std::forward<result_types>(results)...) {}

        when_any_result(when_any_result&&) noexcept = default;
        when_any_result& operator=(when_any_result&&) noexcept = default;
    };
}  // namespace concurrencpp

namespace concurrencpp::details {
    template<class executor_type, class collection_type>
    lazy_result<collection_type> when_all_impl(std::shared_ptr<executor_type> resume_executor, collection_type collection) {
        for (size_t i = 0; i < when_result_helper::size(collection); i++) {
            auto& state_ref = when_result_helper::at(collection, i);
            co_await when_result_helper::when_all_awaitable {state_ref};
        }

        co_await resume_on(resume_executor);
        co_return std::move(collection);
    }
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class executor_type>
    lazy_result<std::tuple<>> when_all(std::shared_ptr<executor_type> resume_executor) {
        if (!static_cast<bool>(resume_executor)) {
            throw std::invalid_argument(details::consts::k_when_all_null_resume_executor_error_msg);
        }

        auto make_lazy_result = []() -> lazy_result<std::tuple<>> {
            co_return std::tuple<>{};
        };

        return make_lazy_result();
    }

    template<class executor_type, class... result_types>
    lazy_result<std::tuple<typename std::decay<result_types>::type...>> when_all(std::shared_ptr<executor_type> resume_executor,
                                                                                 result_types&&... results) {
        details::when_result_helper::throw_if_empty_tuple(details::consts::k_when_all_empty_result_error_msg,
                                                          std::forward<result_types>(results)...);

        if (!static_cast<bool>(resume_executor)) {
            throw std::invalid_argument(details::consts::k_when_all_null_resume_executor_error_msg);
        }

        return details::when_all_impl(resume_executor, std::make_tuple(std::forward<result_types>(results)...));
    }

    template<class executor_type, class iterator_type>
    lazy_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>
    when_all(std::shared_ptr<executor_type> resume_executor, iterator_type begin, iterator_type end) {
        details::when_result_helper::throw_if_empty_range(details::consts::k_when_all_empty_result_error_msg, begin, end);

        if (!static_cast<bool>(resume_executor)) {
            throw std::invalid_argument(details::consts::k_when_all_null_resume_executor_error_msg);
        }

        using type = typename std::iterator_traits<iterator_type>::value_type;

        return details::when_all_impl(resume_executor,
                                      std::vector<type> {std::make_move_iterator(begin), std::make_move_iterator(end)});
    }
}  // namespace concurrencpp

namespace concurrencpp::details {
    template<class executor_type, class tuple_type>
    lazy_result<when_any_result<tuple_type>> when_any_impl(std::shared_ptr<executor_type> resume_executor, tuple_type tuple) {
        const auto completed_index = co_await when_result_helper::when_any_awaitable<tuple_type> {tuple};
        co_await resume_on(resume_executor);
        co_return when_any_result<tuple_type> {completed_index, std::move(tuple)};
    }

    template<class executor_type, class type>
    lazy_result<when_any_result<std::vector<type>>> when_any_impl(std::shared_ptr<executor_type> resume_executor,
                                                                  std::vector<type> vector) {
        const auto completed_index = co_await when_result_helper::when_any_awaitable {vector};
        co_await resume_on(resume_executor);
        co_return when_any_result<std::vector<type>> {completed_index, std::move(vector)};
    }
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class executor_type, class... result_types>
    lazy_result<when_any_result<std::tuple<result_types...>>> when_any(std::shared_ptr<executor_type> resume_executor,
                                                                       result_types&&... results) {
        static_assert(sizeof...(result_types) != 0, "concurrencpp::when_any() - the function must accept at least one result object.");
        details::when_result_helper::throw_if_empty_tuple(details::consts::k_when_any_empty_result_error_msg,
                                                          std::forward<result_types>(results)...);

        if (!static_cast<bool>(resume_executor)) {
            throw std::invalid_argument(details::consts::k_when_any_null_resume_executor_error_msg);
        }

        return details::when_any_impl(resume_executor, std::make_tuple(std::forward<result_types>(results)...));
    }

    template<class executor_type, class iterator_type>
    lazy_result<when_any_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>>
    when_any(std::shared_ptr<executor_type> resume_executor, iterator_type begin, iterator_type end) {
        details::when_result_helper::throw_if_empty_range(details::consts::k_when_any_empty_result_error_msg, begin, end);

        if (begin == end) {
            throw std::invalid_argument(details::consts::k_when_any_empty_range_error_msg);
        }

        if (!static_cast<bool>(resume_executor)) {
            throw std::invalid_argument(details::consts::k_when_any_null_resume_executor_error_msg);
        }

        using type = typename std::iterator_traits<iterator_type>::value_type;

        return details::when_any_impl(resume_executor,
                                      std::vector<type> {std::make_move_iterator(begin), std::make_move_iterator(end)});
    }
}  // namespace concurrencpp

#endif
