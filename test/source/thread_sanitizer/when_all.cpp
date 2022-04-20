#include "concurrencpp/concurrencpp.h"

#include <iostream>

concurrencpp::result<void> test_when_all_tuple(std::shared_ptr<concurrencpp::thread_executor> te);
void test_when_all_vector(std::shared_ptr<concurrencpp::thread_executor> te);

int main() {
    concurrencpp::runtime runtime;
    test_when_all_tuple(runtime.thread_executor()).get();
    test_when_all_vector(runtime.thread_executor());
    return 0;
}

#include "utils/test_generators.h"
#include "utils/test_ready_result.h"

using namespace concurrencpp;
using namespace std::chrono;

result<void> test_when_all_tuple(std::shared_ptr<concurrencpp::thread_executor> te) {
    std::cout << "Testing when_all(result_type&& ... )" << std::endl;

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
    int integer0 = 1234567;
    int integer1 = 7654321;
    int integer2 = 10203040;

    auto int_ref_val_res_0 = te->submit([tp, &integer0]() -> int& {
        std::this_thread::sleep_until(tp);
        return integer0;
    });

    auto int_ref_val_res_1 = te->submit([tp, &integer1]() -> int& {
        std::this_thread::sleep_until(tp);
        return integer1;
    });

    auto int_ref_val_res_2 = te->submit([tp, &integer2]() -> int& {
        std::this_thread::sleep_until(tp);
        return integer2;
    });

    auto int_ref_ex_res_0 = te->submit([tp, &integer0]() -> int& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
        return integer0;
    });

    auto int_ref_ex_res_1 = te->submit([tp, &integer1]() -> int& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
        return integer1;
    });

    auto int_ref_ex_res_2 = te->submit([tp, &integer2]() -> int& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
        return integer2;
    });

    // std::string&
    std::string string0 = "0";
    std::string string1 = "1";
    std::string string2 = "0";

    auto str_ref_val_res_0 = te->submit([tp, &string0]() -> std::string& {
        std::this_thread::sleep_until(tp);
        return string0;
    });

    auto str_ref_val_res_1 = te->submit([tp, &string1]() -> std::string& {
        std::this_thread::sleep_until(tp);
        return string1;
    });

    auto str_ref_val_res_2 = te->submit([tp, &string2]() -> std::string& {
        std::this_thread::sleep_until(tp);
        return string2;
    });

    auto str_ref_ex_res_0 = te->submit([tp, &string0]() -> std::string& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(0);
        return string0;
    });

    auto str_ref_ex_res_1 = te->submit([tp, &string1]() -> std::string& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(1);
        return string1;
    });

    auto str_ref_ex_res_2 = te->submit([tp, &string2]() -> std::string& {
        std::this_thread::sleep_until(tp);
        throw tests::custom_exception(2);
        return string2;
    });

    std::this_thread::sleep_until(tp);

    auto when_all_result = co_await when_all(te,
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

    // int
    tests::test_ready_result(std::move(std::get<0>(when_all_result)), 0);
    tests::test_ready_result(std::move(std::get<1>(when_all_result)), 1);
    tests::test_ready_result(std::move(std::get<2>(when_all_result)), 2);
    tests::test_ready_result_custom_exception(std::move(std::get<3>(when_all_result)), 0);
    tests::test_ready_result_custom_exception(std::move(std::get<4>(when_all_result)), 1);
    tests::test_ready_result_custom_exception(std::move(std::get<5>(when_all_result)), 2);

    // std::string
    tests::test_ready_result(std::move(std::get<6>(when_all_result)), std::string("0"));
    tests::test_ready_result(std::move(std::get<7>(when_all_result)), std::string("1"));
    tests::test_ready_result(std::move(std::get<8>(when_all_result)), std::string("2"));
    tests::test_ready_result_custom_exception(std::move(std::get<9>(when_all_result)), 0);
    tests::test_ready_result_custom_exception(std::move(std::get<10>(when_all_result)), 1);
    tests::test_ready_result_custom_exception(std::move(std::get<11>(when_all_result)), 2);

    // void
    tests::test_ready_result(std::move(std::get<12>(when_all_result)));
    tests::test_ready_result(std::move(std::get<13>(when_all_result)));
    tests::test_ready_result(std::move(std::get<14>(when_all_result)));
    tests::test_ready_result_custom_exception(std::move(std::get<15>(when_all_result)), 0);
    tests::test_ready_result_custom_exception(std::move(std::get<16>(when_all_result)), 1);
    tests::test_ready_result_custom_exception(std::move(std::get<17>(when_all_result)), 2);

    // size_t&
    tests::test_ready_result(std::move(std::get<18>(when_all_result)), std::ref(integer0));
    tests::test_ready_result(std::move(std::get<19>(when_all_result)), std::ref(integer1));
    tests::test_ready_result(std::move(std::get<20>(when_all_result)), std::ref(integer2));
    tests::test_ready_result_custom_exception(std::move(std::get<21>(when_all_result)), 0);
    tests::test_ready_result_custom_exception(std::move(std::get<22>(when_all_result)), 1);
    tests::test_ready_result_custom_exception(std::move(std::get<23>(when_all_result)), 2);

    // std::string&
    tests::test_ready_result(std::move(std::get<24>(when_all_result)), std::ref(string0));
    tests::test_ready_result(std::move(std::get<25>(when_all_result)), std::ref(string1));
    tests::test_ready_result(std::move(std::get<26>(when_all_result)), std::ref(string2));
    tests::test_ready_result_custom_exception(std::move(std::get<27>(when_all_result)), 0);
    tests::test_ready_result_custom_exception(std::move(std::get<28>(when_all_result)), 1);
    tests::test_ready_result_custom_exception(std::move(std::get<29>(when_all_result)), 2);

    std::cout << "================================" << std::endl;
}

struct get_method {
    template<class type>
    result<type> operator()(result<type> res) {
        co_return res.get();
    }
};

template<class type>
result<void> test_when_all_vector_val_impl(std::shared_ptr<concurrencpp::thread_executor> te) {
    const auto tp = system_clock::now() + seconds(2);

    tests::value_gen<type> converter;
    auto results = tests::result_gen<type>::make_result_array(1024, tp, te, converter);

    std::this_thread::sleep_until(tp);

    auto when_all_done = co_await when_all(te, results.begin(), results.end());

    tests::test_result_array(std::move(when_all_done), get_method {}, converter);
}

template<class type>
result<void> test_when_all_vector_ex_impl(std::shared_ptr<concurrencpp::thread_executor> te) {
    const auto tp = system_clock::now() + seconds(2);

    tests::value_gen<type> converter;
    auto results = tests::result_gen<type>::make_exceptional_array(1024, tp, te, converter);

    std::this_thread::sleep_until(tp);

    auto when_all_done = co_await when_all(te, results.begin(), results.end());

    tests::test_exceptional_array(std::move(when_all_done), get_method {});
}

void test_when_all_vector(std::shared_ptr<concurrencpp::thread_executor> te) {
    std::cout << "Testing when_all(begin, end)" << std::endl;

    test_when_all_vector_val_impl<int>(te).get();
    test_when_all_vector_val_impl<std::string>(te).get();
    test_when_all_vector_val_impl<void>(te).get();
    test_when_all_vector_val_impl<int&>(te).get();
    test_when_all_vector_val_impl<std::string&>(te).get();

    test_when_all_vector_ex_impl<int>(te).get();
    test_when_all_vector_ex_impl<std::string>(te).get();
    test_when_all_vector_ex_impl<void>(te).get();
    test_when_all_vector_ex_impl<int&>(te).get();
    test_when_all_vector_ex_impl<std::string&>(te).get();

    std::cout << "================================" << std::endl;
}