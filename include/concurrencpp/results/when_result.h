#ifndef CONCURRENCPP_WHEN_RESULT_H
#define CONCURRENCPP_WHEN_RESULT_H

#include "concurrencpp/errors.h"
#include "concurrencpp/results/make_result.h"

#include <memory>
#include <tuple>
#include <vector>

namespace concurrencpp::details {
    class when_result_helper {

       private:
        template<class type>
        static void throw_if_empty_single(const char* error_message, const result<type>& result) {
            if (!static_cast<bool>(result)) {
                throw errors::empty_result(error_message);
            }
        }

        static void throw_if_empty_impl(const char* error_message) noexcept {
            (void)error_message;
        }

        template<class type, class... result_types>
        static void throw_if_empty_impl(const char* error_message, const result<type>& result, result_types&&... results) {
            throw_if_empty_single(error_message, result);
            throw_if_empty_impl(error_message, std::forward<result_types>(results)...);
        }

        template<class type>
        static result_state<type>& get_state(result<type>& result) noexcept {
            return *result.m_state;
        }

        template<std::size_t... is, typename tuple_type>
        static result_state_base* at_impl(std::index_sequence<is...>, tuple_type& tuple, size_t n) noexcept {
            result_state_base* bases[] = {get_state_base(std::get<is>(tuple))...};
            return bases[n];
        }

       public:
        template<typename tuple_type>
        static result_state_base* at(tuple_type& tuple, size_t n) noexcept {
            auto seq = std::make_index_sequence<std::tuple_size<tuple_type>::value>();
            return at_impl(seq, tuple, n);
        }

        template<class... result_types>
        static void throw_if_empty_tuple(const char* error_message, result_types&&... results) {
            throw_if_empty_impl(error_message, std::forward<result_types>(results)...);
        }

        template<class iterator_type>
        static void throw_if_empty_range(const char* error_message, iterator_type begin, iterator_type end) {
            for (; begin != end; ++begin) {
                throw_if_empty_single(error_message, *begin);
            }
        }
    };

    template<class... result_types>
    class when_all_tuple_state final :
        public when_all_state_base,
        public std::enable_shared_from_this<when_all_tuple_state<result_types...>> {

        using tuple_type = std::tuple<result_types...>;

       private:
        tuple_type m_tuple;
        producer_result_state_ptr<tuple_type> m_state_ptr;

        template<class type>
        void set_state(result<type>& result) noexcept {
            auto& state_ref = when_result_helper::get_state(result);
            state_ref.when_all(this->shared_from_this());
        }

       public:
        when_all_tuple_state(result_types&&... results) noexcept :
            m_tuple(std::forward<result_types>(results)...), m_state_ptr(new result_state<tuple_type>()) {
            m_counter = sizeof...(result_types);
        }

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

            template<class type>
            static result_state_base* get_at(std::vector<type>& vector, size_t i) noexcept {
                return get_state_base(vector[i]);
            }

        template<class given_type>
        void set_state(result<given_type>& result) noexcept {
            auto& state_ref = when_result_helper::get_state(result);
            state_ref.when_all(this->shared_from_this());
        }

       public:
        template<class iterator_type>
        when_all_vector_state(iterator_type begin, iterator_type end) :
            m_vector(std::make_move_iterator(begin), std::make_move_iterator(end)),
            m_state_ptr(new result_state<std::vector<type>>()) {
            m_counter = m_vector.size();
        }

            template<class... types>
            static size_t size(std::tuple<types...>& tuple) noexcept {
                return std::tuple_size_v<std::tuple<types...>>;
            }

           public:
            when_any_awaitable(result_types& results) noexcept : m_results(results) {}

            bool await_ready() const noexcept {
                return false;
            }

            void await_suspend(coroutine_handle<void> coro_handle) {
                m_promise = std::make_shared<when_any_context>(coro_handle);

                const auto range_length = size(m_results);
                for (size_t i = 0; i < range_length; i++) {
                    if (m_promise->fulfilled()) {
                        return;
                    }

                    auto state_ptr = get_at(m_results, i);
                    const auto status = state_ptr->when_any(m_promise);
                    if (status == result_state_base::pc_state::producer_done) {
                        m_promise->try_resume(state_ptr);
                        return;
                    }
                }
            }

            size_t await_resume() noexcept {
                const auto completed_result_state = m_promise->completed_result();
                auto completed_result_index = std::numeric_limits<size_t>::max();

                const auto range_length = size(m_results);
                for (size_t i = 0; i < range_length; i++) {
                    auto state_ptr = get_at(m_results, i);
                    state_ptr->try_rewind_consumer();
                    if (completed_result_state == state_ptr) {
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
    template<class... result_types>
    class when_any_tuple_state final :
        public when_any_state_base,
        public std::enable_shared_from_this<when_any_tuple_state<result_types...>> {

        using tuple_type = std::tuple<result_types...>;

       private:
        tuple_type m_results;
        producer_result_state_ptr<when_any_result<tuple_type>> m_state_ptr;

        template<size_t index>
        std::pair<when_any_status, size_t> set_state_impl(
            std::unique_lock<std::recursive_mutex>& lock) noexcept {  // should be called under a lock.
            assert(lock.owns_lock());
            (void)lock;

            auto& result = std::get<index>(m_results);
            auto& state_ref = when_result_helper::get_state(result);
            const auto status = state_ref.when_any(this->shared_from_this(), index);

            if (status == when_any_status::result_ready) {
                return {status, index};
            }

            const auto res = set_state_impl<index + 1>(lock);

            if (res.first == when_any_status::result_ready) {
                state_ref.try_rewind_consumer();
            }

            return res;
        }

        template<>
        std::pair<when_any_status, size_t> set_state_impl<sizeof...(result_types)>(
            std::unique_lock<std::recursive_mutex>& lock) noexcept {
            (void)lock;
            return {when_any_status::set, static_cast<size_t>(-1)};
        }

        template<size_t index>
        void unset_state(std::unique_lock<std::recursive_mutex>& lock, size_t done_index) noexcept {
            assert(lock.owns_lock());
            (void)lock;
            if (index != done_index) {
                auto& state_ref = when_result_helper::get_state(std::get<index>(m_results));
                state_ref.try_rewind_consumer();
            }

        for (size_t i = 0; i < std::tuple_size_v<decltype(tuple)>; i++) {
            auto state_ptr = when_result_helper::at(tuple, i);
            co_await when_result_helper::when_all_awaitable {*state_ptr};
        }

        template<>
        void unset_state<sizeof...(result_types)>(std::unique_lock<std::recursive_mutex>& lock, size_t done_index) noexcept {
            (void)lock;
            (void)done_index;
        }

        void complete_promise(std::unique_lock<std::recursive_mutex>& lock, size_t index) noexcept {
            assert(lock.owns_lock());
            (void)lock;

            auto state = std::move(m_state_ptr);
            auto results = std::move(m_results);
            lock.unlock();

            state->set_result(index, std::move(results));
        }

       public:
        when_any_tuple_state(result_types&&... results) :
            m_results(std::forward<result_types>(results)...), m_state_ptr(new result_state<when_any_result<tuple_type>>()) {}

        void on_result_ready(size_t index) noexcept override {
            if (m_fulfilled.exchange(true, std::memory_order_relaxed)) {
                return;
            }

            std::unique_lock<std::recursive_mutex> lock(m_lock);
            unset_state<0>(lock, index);
            complete_promise(lock, index);
        }

        void set_state() noexcept {
            std::unique_lock<std::recursive_mutex> lock(m_lock);
            const auto [status, index] = set_state_impl<0>(lock);

            if (status == when_any_status::result_ready) {
                complete_promise(lock, index);
            }
        }

        result<when_any_result<tuple_type>> get_result() noexcept {
            return {m_state_ptr.get()};
        }
    };

    template<class type>
    class when_any_vector_state final : public when_any_state_base, public std::enable_shared_from_this<when_any_vector_state<type>> {

       private:
        std::vector<type> m_results;
        producer_result_state_ptr<when_any_result<std::vector<type>>> m_state_ptr;

        void unset_state(std::unique_lock<std::recursive_mutex>& lock) noexcept {
            assert(lock.owns_lock());
            (void)lock;

            for (auto& result : m_results) {
                assert(static_cast<bool>(result));
                auto& state_ref = when_result_helper::get_state(result);
                state_ref.try_rewind_consumer();
            }
        }

        void complete_promise(std::unique_lock<std::recursive_mutex>& lock, size_t index) noexcept {
            assert(lock.owns_lock());
            (void)lock;

            auto results = std::move(m_results);
            auto state = std::move(m_state_ptr);
            lock.unlock();

            state->set_result(index, std::move(results));
        }

       public:
        template<class iterator_type>
        when_any_vector_state(iterator_type begin, iterator_type end) :
            m_results(std::make_move_iterator(begin), std::make_move_iterator(end)),
            m_state_ptr(new result_state<when_any_result<std::vector<type>>>()) {}

    template<class iterator_type>
    result<std::vector<typename std::iterator_traits<iterator_type>::value_type>> when_all_impl(iterator_type begin,
                                                                                                iterator_type end) {
        using type = typename std::iterator_traits<iterator_type>::value_type;

        if (begin == end) {
            co_return std::vector<type> {};
        }

        void set_state() noexcept {
            std::unique_lock<std::recursive_mutex> lock(m_lock);
            for (size_t i = 0; i < m_results.size(); i++) {
                if (m_fulfilled.load(std::memory_order_relaxed)) {
                    return;
                }

                auto& state_ref = when_result_helper::get_state(m_results[i]);
                const auto res = state_ref.when_any(this->shared_from_this(), i);

        for (auto& result : vector) {
            result = co_await result.resolve();
        }

        co_return std::move(vector);
    }
}  // namespace concurrencpp::details

namespace concurrencpp {
    inline result<std::tuple<>> when_all() {
        return make_ready_result<std::tuple<>>();
    }

    template<class... result_types>
    result<std::tuple<typename std::decay<result_types>::type...>> when_all(result_types&&... results) {
        details::when_result_helper::throw_if_empty_tuple(details::consts::k_when_all_empty_result_error_msg,
                                                          std::forward<result_types>(results)...);

        auto when_all_state = std::make_shared<details::when_all_tuple_state<typename std::decay<result_types>::type...>>(
            std::forward<result_types>(results)...);
        auto result = when_all_state->get_result();
        when_all_state->set_state();
        return std::move(result);
    }

    template<class iterator_type>
    result<std::vector<typename std::iterator_traits<iterator_type>::value_type>> when_all(iterator_type begin, iterator_type end) {
        details::when_result_helper::throw_if_empty_range(details::consts::k_when_all_empty_result_error_msg, begin, end);

        return details::when_all_impl(begin, end);
    }
}  // namespace concurrencpp

namespace concurrencpp::details {
    template<class... result_types>
    result<when_any_result<std::tuple<result_types...>>> when_any_impl(result_types&&... results) {
        using tuple_type = std::tuple<result_types...>;
        tuple_type tuple = std::make_tuple(std::forward<result_types>(results)...);

        const auto completed_index = co_await when_result_helper::when_any_awaitable<tuple_type> {tuple};
        co_return when_any_result<tuple_type> {completed_index, std::move(tuple)};
    }

    template<class iterator_type>
    result<when_any_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>> when_any_impl(iterator_type begin,
                                                                                                                 iterator_type end) {
        using type = typename std::iterator_traits<iterator_type>::value_type;

        std::vector<type> vector {std::make_move_iterator(begin), std::make_move_iterator(end)};

        const auto completed_index = co_await when_result_helper::when_any_awaitable {vector};
        co_return when_any_result<std::vector<type>> {completed_index, std::move(vector)};
    }
}  // namespace concurrencpp::details

namespace concurrencpp {
    template<class... result_types>
    result<when_any_result<std::tuple<result_types...>>> when_any(result_types&&... results) {
        static_assert(sizeof...(result_types) != 0, "concurrencpp::when_any() - the function must accept at least one result object.");
        details::when_result_helper::throw_if_empty_tuple(details::consts::k_when_any_empty_result_error_msg,
                                                          std::forward<result_types>(results)...);

        auto when_any_state = std::make_shared<details::when_any_tuple_state<result_types...>>(std::forward<result_types>(results)...);
        auto result = when_any_state->get_result();
        when_any_state->set_state();
        return std::move(result);
    }

    template<class iterator_type>
    result<when_any_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>> when_any(iterator_type begin,
                                                                                                            iterator_type end) {
        details::when_result_helper::throw_if_empty_range(details::consts::k_when_any_empty_result_error_msg, begin, end);

        if (begin == end) {
            throw std::invalid_argument(details::consts::k_when_any_empty_range_error_msg);
        }

        return details::when_any_impl(begin, end);
    }
}  // namespace concurrencpp

#endif
