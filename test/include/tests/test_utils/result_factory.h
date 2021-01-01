#ifndef CONCURRENCPP_RESULT_FACTORY_H
#define CONCURRENCPP_RESULT_FACTORY_H

#include <string>
#include <vector>
#include <numeric>
#include <stdexcept>

namespace concurrencpp::tests {
    struct test_exception : public std::runtime_error {
        test_exception() : runtime_error("test exception.") {}
    };

    template<class type>
    struct result_factory {
        static type get() {
            return type();
        }

        static type throw_ex() {
            throw test_exception {};
            return get();
        }

        static result<type> make_ready() {
            return make_ready_result<type>(get());
        }

        static result<type> make_exceptional() {
            return make_exceptional_result<type>(test_exception {});
        }
    };

    template<>
    struct result_factory<int> {
        static int get() {
            return 123456789;
        }

        static std::vector<int> get_many(size_t count) {
            std::vector<int> res(count);
            std::iota(res.begin(), res.end(), 0);
            return res;
        }

        static int throw_ex() {
            throw test_exception {};
            return get();
        }

        static result<int> make_ready() {
            return make_ready_result<int>(get());
        }

        static result<int> make_exceptional() {
            return make_exceptional_result<int>(test_exception {});
        }
    };

    template<>
    struct result_factory<std::string> {
        static std::string get() {
            return "abcdefghijklmnopqrstuvwxyz123456789!@#$%^&*()";
        }

        static std::vector<std::string> get_many(size_t count) {
            std::vector<std::string> res;
            res.reserve(count);

            for (size_t i = 0; i < count; i++) {
                res.emplace_back(std::to_string(i));
            }

            return res;
        }

        static std::string throw_ex() {
            throw test_exception {};
            return get();
        }

        static result<std::string> make_ready() {
            return make_ready_result<std::string>(get());
        }

        static result<std::string> make_exceptional() {
            return make_exceptional_result<std::string>(test_exception {});
        }
    };

    template<>
    struct result_factory<void> {
        static void get() {}

        static void throw_ex() {
            throw test_exception {};
        }

        static std::vector<std::nullptr_t> get_many(size_t count) {
            return std::vector<std::nullptr_t> {count};
        }

        static result<void> make_ready() {
            return make_ready_result<void>();
        }

        static result<void> make_exceptional() {
            return make_exceptional_result<void>(test_exception {});
        }
    };

    template<>
    struct result_factory<int&> {
        static int& get() {
            static int i = 0;
            return i;
        }

        static std::vector<std::reference_wrapper<int>> get_many(size_t count) {
            static std::vector<int> s_res(64);
            std::vector<std::reference_wrapper<int>> res;
            res.reserve(count);
            for (size_t i = 0; i < count; i++) {
                res.emplace_back(s_res[i % 64]);
            }

            return res;
        }

        static int& throw_ex() {
            throw test_exception {};
            return get();
        }

        static result<int&> make_ready() {
            return make_ready_result<int&>(get());
        }

        static result<int&> make_exceptional() {
            return make_exceptional_result<int&>(test_exception {});
        }
    };

    template<>
    struct result_factory<std::string&> {
        static std::string& get() {
            static std::string str;
            return str;
        }

        static std::vector<std::reference_wrapper<std::string>> get_many(size_t count) {
            static std::vector<std::string> s_res(64);
            std::vector<std::reference_wrapper<std::string>> res;
            res.reserve(count);
            for (size_t i = 0; i < count; i++) {
                res.emplace_back(s_res[i % 64]);
            }

            return res;
        }

        static std::string& throw_ex() {
            throw test_exception {};
            return get();
        }

        static result<std::string&> make_ready() {
            return make_ready_result<std::string&>(get());
        }

        static result<std::string&> make_exceptional() {
            return make_exceptional_result<std::string&>(test_exception {});
        }
    };
}  // namespace concurrencpp::tests

#endif
