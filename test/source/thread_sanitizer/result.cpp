#include "concurrencpp/concurrencpp.h"

#include <iostream>

void test_result_get(std::shared_ptr<concurrencpp::thread_executor> te);
void test_result_wait(std::shared_ptr<concurrencpp::thread_executor> te);
void test_result_wait_for(std::shared_ptr<concurrencpp::thread_executor> te);
void test_result_await(std::shared_ptr<concurrencpp::thread_executor> te);
void test_result_resolve(std::shared_ptr<concurrencpp::thread_executor> te);

int main() {
    std::cout << "Starting concurrencpp::result test" << std::endl;

    concurrencpp::runtime runtime;
    const auto thread_executor = runtime.thread_executor();

    test_result_get(thread_executor);
    test_result_wait(thread_executor);
    test_result_wait_for(thread_executor);
    test_result_await(thread_executor);
    test_result_resolve(thread_executor);
}

#include "utils/test_generators.h"
#include "utils/test_ready_result.h"

using namespace concurrencpp;
using namespace std::chrono;

namespace concurrencpp::tests {
    template<class type, class method_functor>
    void test_result_method_val(std::shared_ptr<thread_executor> te, method_functor&& tested_method, value_gen<type> converter) {
        const auto tp = system_clock::now() + seconds(2);
        auto results = result_gen<type>::make_result_array(1024, tp, te, converter);

        std::this_thread::sleep_until(tp);

        test_result_array(std::move(results), tested_method, converter);
    }

    template<class type, class method_functor>
    void test_result_method_ex(std::shared_ptr<thread_executor> te, method_functor&& tested_method, value_gen<type> converter) {
        const auto tp = system_clock::now() + seconds(2);
        auto results = result_gen<type>::make_exceptional_array(1024, tp, te, converter);

        std::this_thread::sleep_until(tp);

        test_exceptional_array(std::move(results), tested_method);
    }
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    struct get_method {
        template<class type>
        result<type> operator()(result<type> res) {
            co_return res.get();
        }
    };

    struct wait_method {
        template<class type>
        result<type> operator()(result<type> res) {
            res.wait();
            return std::move(res);
        }
    };

    struct wait_for_method {
        template<class type>
        result<type> operator()(result<type> res) {
            while (res.wait_for(milliseconds(5)) == result_status::idle) {
                // do nothing.
            }

            return std::move(res);
        }
    };

    class await_method {

       private:
        template<class type>
        result<type> await_task(result<type> res) {
            co_return co_await res;
        }

       public:
        template<class type>
        result<type> operator()(result<type> res) {
            auto wrapper_res = await_task(std::move(res));
            wrapper_res.wait();
            return std::move(wrapper_res);
        }
    };

    class resolve_method {

       private:
        template<class type>
        result<result<type>> await_task(result<type> res) {
            co_return co_await res.resolve();
        }

       public:
        template<class type>
        result<type> operator()(result<type> res) {
            auto wrapper_res = await_task(std::move(res));
            return wrapper_res.get();
        }
    };

    template<class tested_method>
    void test_result_method(std::shared_ptr<thread_executor> te, tested_method&& method) {
        tests::test_result_method_val<int>(te, method, value_gen<int> {});
        tests::test_result_method_val<std::string>(te, method, value_gen<std::string> {});
        tests::test_result_method_val<void>(te, method, value_gen<void> {});
        tests::test_result_method_val<int&>(te, method, value_gen<int&> {});
        tests::test_result_method_val<std::string&>(te, method, value_gen<std::string&> {});

        tests::test_result_method_ex<int>(te, method, value_gen<int> {});
        tests::test_result_method_ex<std::string>(te, method, value_gen<std::string> {});
        tests::test_result_method_ex<void>(te, method, value_gen<void> {});
        tests::test_result_method_ex<int&>(te, method, value_gen<int&> {});
        tests::test_result_method_ex<std::string&>(te, method, value_gen<std::string&> {});
    }
}  // namespace concurrencpp::tests

void test_result_get(std::shared_ptr<thread_executor> te) {
    std::cout << "Testing result::get()" << std::endl;

    tests::test_result_method(te, tests::get_method {});

    std::cout << "================================" << std::endl;
}

void test_result_wait(std::shared_ptr<thread_executor> te) {
    std::cout << "Testing result::wait()" << std::endl;

    tests::test_result_method(te, tests::wait_method {});

    std::cout << "================================" << std::endl;
}

void test_result_wait_for(std::shared_ptr<thread_executor> te) {
    std::cout << "Testing result::wait_for()" << std::endl;

    tests::test_result_method(te, tests::wait_for_method {});

    std::cout << "================================" << std::endl;
}

void test_result_await(std::shared_ptr<thread_executor> te) {
    std::cout << "Testing result::await()" << std::endl;

    tests::test_result_method(te, tests::await_method {});

    std::cout << "================================" << std::endl;
}

void test_result_resolve(std::shared_ptr<concurrencpp::thread_executor> te) {
    std::cout << "Testing result::resolve()" << std::endl;

    tests::test_result_method(te, tests::resolve_method {});

    std::cout << "================================" << std::endl;
}
