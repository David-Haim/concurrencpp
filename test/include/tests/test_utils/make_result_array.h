#ifndef CONCURRENCPP_MAKE_RESULT_ARRAY_H
#define CONCURRENCPP_MAKE_RESULT_ARRAY_H

#include "concurrencpp/concurrencpp.h"
#include "tests/test_utils/test_ready_result.h"

#include <array>
#include <chrono>

namespace concurrencpp::tests {
    template<class type, class converter_type>
    std::vector<result<type>> make_result_array(size_t count,
                                                std::chrono::time_point<std::chrono::system_clock> tp,
                                                std::shared_ptr<concurrencpp::thread_executor> te,
                                                converter_type converter) {
        std::vector<result<type>> results;
        results.reserve(count);

        for (size_t i = 0; i < count; i++) {
            results.emplace_back(te->submit([tp, i, &converter]() mutable -> type {
                std::this_thread::sleep_until(tp);
                return converter(i);
            }));
        }

        return results;
    }

    template<class type, class converter_type>
    std::vector<result<type>> make_exceptional_array(size_t count,
                                                     std::chrono::time_point<std::chrono::system_clock> tp,
                                                     std::shared_ptr<concurrencpp::thread_executor> te,
                                                     converter_type converter) {
        std::vector<result<type>> results;
        results.reserve(count);

        for (size_t i = 0; i < count; i++) {
            results.emplace_back(te->submit([tp, i, converter]() mutable -> type {
                std::this_thread::sleep_until(tp);

                throw costume_exception(i);

                return converter(i);
            }));
        }

        return results;
    }

    template<class type, class consumer_type, class converter_type>
    void test_result_array(std::vector<result<type>> results, consumer_type&& consumer, converter_type&& converter) {
        for (size_t i = 0; i < results.size(); i++) {
            if constexpr (!std::is_same_v<void, type>) {
                test_ready_result(consumer(std::move(results[i])), converter(i));
            } else {
                test_ready_result(consumer(std::move(results[i])));
            }
        }
    }

    template<class type, class consumer_type>
    void test_exceptional_array(std::vector<result<type>> results, consumer_type&& consumer) {
        for (size_t i = 0; i < results.size(); i++) {
            test_ready_result_costume_exception(consumer(std::move(results[i])), i);
        }
    }

    template<class type, class consumer_type, class converter_type>
    void test_shared_result_array(std::vector<shared_result<type>> results, consumer_type&& consumer, converter_type&& converter) {
        for (size_t i = 0; i < results.size(); i++) {
            if constexpr (!std::is_same_v<void, type>) {
                test_ready_result(consumer(std::move(results[i])), converter(i));
            } else {
                test_ready_result(consumer(std::move(results[i])));
            }
        }
    }

    template<class type, class consumer_type>
    void test_shared_result_exceptional_array(std::vector<shared_result<type>> results, consumer_type&& consumer) {
        for (size_t i = 0; i < results.size(); i++) {
            test_ready_result_costume_exception(consumer(std::move(results[i])), i);
        }
    }

    template<class type>
    struct converter {
        type operator()(size_t) const noexcept {
            return 0;
        }
    };

    template<>
    struct converter<size_t> {
        size_t operator()(size_t i) const noexcept {
            return i;
        }
    };

    template<>
    struct converter<std::string> {
        std::string operator()(size_t i) const {
            return std::to_string(i);
        }
    };

    template<>
    struct converter<void> {
        void operator()(size_t) const noexcept {}
    };

    template<>
    class converter<size_t&> {

       private:
        std::shared_ptr<std::array<size_t, 16>> array = std::make_shared<std::array<size_t, 16>>();

       public:
        size_t& operator()(size_t i) const {
            return (*array)[i % 16];
        }
    };

    template<>
    class converter<std::string&> {

       private:
        std::shared_ptr<std::array<std::string, 16>> array = std::make_shared<std::array<std::string, 16>>();

       public:
        std::string& operator()(size_t i) {
            return (*array)[i % 16];
        }
    };
}  // namespace concurrencpp::tests

#endif