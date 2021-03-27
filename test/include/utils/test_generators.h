#ifndef CONCURRENCPP_VALUE_GEN_H
#define CONCURRENCPP_VALUE_GEN_H

#include "utils/custom_exception.h"

#include <array>
#include <string>
#include <vector>
#include <stdexcept>

namespace concurrencpp::tests {
    struct test_exception : public std::runtime_error {
        test_exception() : runtime_error("test exception.") {}
    };

    template<class type>
    struct value_gen {
        static type default_value() {
            return type();
        }

        static type throw_ex() {
            throw test_exception {};
            return default_value();
        }

        type value_of(size_t index) {
            return {};
        }
    };

    template<>
    struct value_gen<int> {

        int value_of(size_t index) {
            return static_cast<int>(index);
        }

        static int default_value() {
            return 123456789;
        }

        static int throw_ex() {
            throw test_exception {};
            return default_value();
        }
    };

    template<>
    struct value_gen<std::string> {
        static std::string default_value() {
            return "abcdefghijklmnopqrstuvwxyz123456789!@#$%^&*()";
        }

        std::string value_of(size_t i) const {
            return std::to_string(i);
        }

        static std::string throw_ex() {
            throw test_exception {};
            return default_value();
        }
    };

    template<>
    struct value_gen<void> {
        static void default_value() {}

        static void throw_ex() {
            throw test_exception {};
        }

        void value_of(size_t) const noexcept {}
    };

    template<>
    struct value_gen<int&> {

       private:
        std::shared_ptr<std::array<int, 16>> m_array = std::make_shared<std::array<int, 16>>();

       public:
        static int& default_value() {
            static int i = 0;
            return i;
        }

        int& value_of(size_t i) const {
            return (*m_array)[i % 16];
        }

        static int& throw_ex() {
            throw test_exception {};
            return default_value();
        }
    };

    template<>
    struct value_gen<std::string&> {

       private:
        std::shared_ptr<std::array<std::string, 16>> m_array = std::make_shared<std::array<std::string, 16>>();

       public:
        static std::string& default_value() {
            static std::string str;
            return str;
        }

        std::string& value_of(size_t i) {
            return (*m_array)[i % 16];
        }

        static std::string& throw_ex() {
            throw test_exception {};
            return default_value();
        }
    };

    template<class type>
    struct result_gen {
        static result<type> ready() {
            co_return value_gen<type>::default_value();
        }

        static result<type> exceptional() {
            throw test_exception {};
            co_return value_gen<type>::default_value();
        }

        static std::vector<result<type>> make_result_array(size_t count,
                                                           std::chrono::time_point<std::chrono::system_clock> tp,
                                                           std::shared_ptr<concurrencpp::thread_executor> te,
                                                           value_gen<type> gen) {
            std::vector<result<type>> results;
            results.reserve(count);

            for (size_t i = 0; i < count; i++) {
                results.emplace_back(te->submit([tp, i, gen]() mutable -> type {
                    std::this_thread::sleep_until(tp);
                    return gen.value_of(i);
                }));
            }

            return results;
        }

        static std::vector<result<type>> make_exceptional_array(size_t count,
                                                                std::chrono::time_point<std::chrono::system_clock> tp,
                                                                std::shared_ptr<concurrencpp::thread_executor> te,
                                                                value_gen<type> gen) {
            std::vector<result<type>> results;
            results.reserve(count);

            for (size_t i = 0; i < count; i++) {
                results.emplace_back(te->submit([tp, i, gen]() mutable -> type {
                    std::this_thread::sleep_until(tp);

                    throw custom_exception(i);

                    return gen.value_of(i);
                }));
            }

            return results;
        }
    };

}  // namespace concurrencpp::tests

#endif
