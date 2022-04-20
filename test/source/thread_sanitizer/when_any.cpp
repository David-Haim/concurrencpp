#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <random>

concurrencpp::result<void> test_when_any_tuple(std::shared_ptr<concurrencpp::thread_executor> te);
void test_when_any_vector(std::shared_ptr<concurrencpp::thread_executor> te);

int main() {
    concurrencpp::runtime runtime;
    test_when_any_tuple(runtime.thread_executor()).get();
    test_when_any_vector(runtime.thread_executor());
    return 0;
}

#include "utils/test_generators.h"
#include "utils/test_ready_result.h"

using namespace concurrencpp;
using namespace std::chrono;

const int integer0 = 1234567;
const int integer1 = 7654321;
const int integer2 = 10203040;

const std::string string0 = "0";
const std::string string1 = "1";
const std::string string2 = "0";

result<void> test_when_any_tuple(std::shared_ptr<concurrencpp::thread_executor> te) {
    std::cout << "Testing when_any(result_type&& ... )" << std::endl;

    const auto tp = system_clock::now() + milliseconds(250);

    // int
    auto int_val_res_0 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        return 0;
    });

    auto int_val_res_1 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        return 1;
    });

    auto int_val_res_2 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        return 2;
    });

    auto int_ex_res_0 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
        return int();
    });

    auto int_ex_res_1 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
        return int();
    });

    auto int_ex_res_2 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
        return int();
    });

    // std::string
    auto str_val_res_0 = te->submit([tp]() -> std::string {
        std::this_thread::sleep_until(tp);
        return "0";
    });

    auto str_val_res_1 = te->submit([tp]() -> std::string {
        std::this_thread::sleep_until(tp);
        return "1";
    });

    auto str_val_res_2 = te->submit([tp]() -> std::string {
        std::this_thread::sleep_until(tp);
        return "2";
    });

    auto str_ex_res_0 = te->submit([tp]() -> std::string {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
        return std::string();
    });

    auto str_ex_res_1 = te->submit([tp]() -> std::string {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
        return std::string();
    });

    auto str_ex_res_2 = te->submit([tp]() -> std::string {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
        return std::string();
    });

    // void
    auto void_val_res_0 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
    });

    auto void_val_res_1 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
    });

    auto void_val_res_2 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
    });

    auto void_ex_res_0 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
    });

    auto void_ex_res_1 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
    });

    auto void_ex_res_2 = te->submit([tp] {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
    });

    // int&
    auto int_ref_val_res_0 = te->submit([tp]() -> const int& {
        std::this_thread::sleep_until(tp);
        return integer0;
    });

    auto int_ref_val_res_1 = te->submit([tp]() -> const int& {
        std::this_thread::sleep_until(tp);
        return integer1;
    });

    auto int_ref_val_res_2 = te->submit([tp]() -> const int& {
        std::this_thread::sleep_until(tp);
        return integer2;
    });

    auto int_ref_ex_res_0 = te->submit([tp]() -> const int& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
        return integer0;
    });

    auto int_ref_ex_res_1 = te->submit([tp]() -> const int& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
        return integer1;
    });

    auto int_ref_ex_res_2 = te->submit([tp]() -> const int& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
        return integer2;
    });

    // std::string&
    auto str_ref_val_res_0 = te->submit([tp]() -> const std::string& {
        std::this_thread::sleep_until(tp);
        return string0;
    });

    auto str_ref_val_res_1 = te->submit([tp]() -> const std::string& {
        std::this_thread::sleep_until(tp);
        return string1;
    });

    auto str_ref_val_res_2 = te->submit([tp]() -> const std::string& {
        std::this_thread::sleep_until(tp);
        return string2;
    });

    auto str_ref_ex_res_0 = te->submit([tp]() -> const std::string& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
        return string0;
    });

    auto str_ref_ex_res_1 = te->submit([tp]() -> const std::string& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
        return string1;
    });

    auto str_ref_ex_res_2 = te->submit([tp]() -> const std::string& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
        return string2;
    });

    std::this_thread::sleep_until(tp);

    auto when_any_result = co_await when_any(te,
                                             std::move(int_val_res_0),
                                             std::move(int_val_res_1),
                                             std::move(int_val_res_2),

                                             std::move(int_ex_res_0),
                                             std::move(int_ex_res_1),
                                             std::move(int_ex_res_2),

                                             std::move(str_val_res_0),
                                             std::move(str_val_res_1),
                                             std::move(str_val_res_2),

                                             std::move(str_ex_res_0),
                                             std::move(str_ex_res_1),
                                             std::move(str_ex_res_2),

                                             std::move(void_val_res_0),
                                             std::move(void_val_res_1),
                                             std::move(void_val_res_2),

                                             std::move(void_ex_res_0),
                                             std::move(void_ex_res_1),
                                             std::move(void_ex_res_2),

                                             std::move(int_ref_val_res_0),
                                             std::move(int_ref_val_res_1),
                                             std::move(int_ref_val_res_2),

                                             std::move(int_ref_ex_res_0),
                                             std::move(int_ref_ex_res_1),
                                             std::move(int_ref_ex_res_2),

                                             std::move(str_ref_val_res_0),
                                             std::move(str_ref_val_res_1),
                                             std::move(str_ref_val_res_2),

                                             std::move(str_ref_ex_res_0),
                                             std::move(str_ref_ex_res_1),
                                             std::move(str_ref_ex_res_2));

    switch (when_any_result.index) {
        // int - val
        case 0: {
            tests::test_ready_result(std::move(std::get<0>(when_any_result.results)), 0);
            break;
        }

        case 1: {
            tests::test_ready_result(std::move(std::get<1>(when_any_result.results)), 1);
            break;
        }

        case 2: {
            tests::test_ready_result(std::move(std::get<2>(when_any_result.results)), 2);
            break;
        }

        // int - ex
        case 3: {
            tests::test_ready_result_custom_exception(std::move(std::get<3>(when_any_result.results)), 0);
            break;
        }

        case 4: {
            tests::test_ready_result_custom_exception(std::move(std::get<4>(when_any_result.results)), 1);
            break;
        }

        case 5: {
            tests::test_ready_result_custom_exception(std::move(std::get<5>(when_any_result.results)), 2);
            break;
        }

        // str - val
        case 6: {
            tests::test_ready_result(std::move(std::get<6>(when_any_result.results)), std::string("0"));
            break;
        }

        case 7: {
            tests::test_ready_result(std::move(std::get<7>(when_any_result.results)), std::string("1"));
            break;
        }

        case 8: {
            tests::test_ready_result(std::move(std::get<8>(when_any_result.results)), std::string("2"));
            break;
        }

        // str - ex
        case 9: {
            tests::test_ready_result_custom_exception(std::move(std::get<9>(when_any_result.results)), 0);
            break;
        }

        case 10: {
            tests::test_ready_result_custom_exception(std::move(std::get<10>(when_any_result.results)), 1);
            break;
        }

        case 11: {
            tests::test_ready_result_custom_exception(std::move(std::get<11>(when_any_result.results)), 2);
            break;
        }

        // void - val
        case 12: {
            tests::test_ready_result(std::move(std::get<12>(when_any_result.results)));
            break;
        }

        case 13: {
            tests::test_ready_result(std::move(std::get<13>(when_any_result.results)));
            break;
        }

        case 14: {
            tests::test_ready_result(std::move(std::get<14>(when_any_result.results)));
            break;
        }

        // void - ex
        case 15: {
            tests::test_ready_result_custom_exception(std::move(std::get<15>(when_any_result.results)), 0);
            break;
        }

        case 16: {
            tests::test_ready_result_custom_exception(std::move(std::get<16>(when_any_result.results)), 1);
            break;
        }

        case 17: {
            tests::test_ready_result_custom_exception(std::move(std::get<17>(when_any_result.results)), 2);
            break;
        }

        // int& - val
        case 18: {
            tests::test_ready_result(std::move(std::get<18>(when_any_result.results)), std::ref(integer0));
            break;
        }

        case 19: {
            tests::test_ready_result(std::move(std::get<19>(when_any_result.results)), std::ref(integer1));
            break;
        }

        case 20: {
            tests::test_ready_result(std::move(std::get<20>(when_any_result.results)), std::ref(integer2));
            break;
        }

        // int& - ex
        case 21: {
            tests::test_ready_result_custom_exception(std::move(std::get<21>(when_any_result.results)), 0);
            break;
        }

        case 22: {
            tests::test_ready_result_custom_exception(std::move(std::get<22>(when_any_result.results)), 1);
            break;
        }

        case 23: {
            tests::test_ready_result_custom_exception(std::move(std::get<23>(when_any_result.results)), 2);
            break;
        }

        // std::string& - val
        case 24: {
            tests::test_ready_result(std::move(std::get<24>(when_any_result.results)), std::ref(string0));
            break;
        }

        case 25: {
            tests::test_ready_result(std::move(std::get<25>(when_any_result.results)), std::ref(string1));
            break;
        }

        case 26: {
            tests::test_ready_result(std::move(std::get<26>(when_any_result.results)), std::ref(string2));
            break;
        }

        // std::string& - ex
        case 27: {
            tests::test_ready_result_custom_exception(std::move(std::get<27>(when_any_result.results)), 0);
            break;
        }

        case 28: {
            tests::test_ready_result_custom_exception(std::move(std::get<28>(when_any_result.results)), 1);
            break;
        }

        case 29: {
            tests::test_ready_result_custom_exception(std::move(std::get<29>(when_any_result.results)), 2);
            break;
        }
    }

    std::cout << "================================" << std::endl;
}

template<class type>
result<void> test_when_any_vector_val_impl(std::shared_ptr<concurrencpp::thread_executor> te) {
    const auto tp = system_clock::now() + seconds(2);

    tests::value_gen<type> converter;
    auto results = tests::result_gen<type>::make_result_array(1024, tp, te, converter);

    std::this_thread::sleep_until(tp);

    auto when_any_done = co_await when_any(te, results.begin(), results.end());

    if constexpr (!std::is_same_v<void, type>) {
        tests::test_ready_result(std::move(when_any_done.results[when_any_done.index]), converter.value_of(when_any_done.index));

    } else {
        tests::test_ready_result(std::move(when_any_done.results[when_any_done.index]));
    }
}

template<class type>
result<void> test_when_any_vector_ex_impl(std::shared_ptr<concurrencpp::thread_executor> te) {
    const auto tp = system_clock::now() + seconds(2);

    tests::value_gen<type> converter;
    auto results = tests::result_gen<type>::make_exceptional_array(1024, tp, te, converter);

    std::this_thread::sleep_until(tp);

    auto when_any_done = co_await when_any(te, results.begin(), results.end());

    tests::test_ready_result_custom_exception(std::move(when_any_done.results[when_any_done.index]), when_any_done.index);
}

void test_when_any_vector(std::shared_ptr<concurrencpp::thread_executor> te) {
    std::cout << "Testing when_any(begin, end)" << std::endl;

    test_when_any_vector_val_impl<int>(te).get();
    test_when_any_vector_val_impl<std::string>(te).get();
    test_when_any_vector_val_impl<void>(te).get();
    test_when_any_vector_val_impl<int&>(te).get();
    test_when_any_vector_val_impl<std::string&>(te).get();

    test_when_any_vector_ex_impl<int>(te).get();
    test_when_any_vector_ex_impl<std::string>(te).get();
    test_when_any_vector_ex_impl<void>(te).get();
    test_when_any_vector_ex_impl<int&>(te).get();
    test_when_any_vector_ex_impl<std::string&>(te).get();

    std::cout << "================================" << std::endl;
}