#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tests/test_utils/test_ready_result.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/random.h"
#include "helpers/object_observer.h"

#include <string>

namespace concurrencpp::tests {
template<class type>
void test_result_promise_constructor_impl();
void test_result_promise_constructor();

template<class type>
void test_result_promise_destructor_impl();
void test_result_promise_RAII_impl();
void test_result_promise_destructor();

template<class type>
void test_result_promise_get_result_impl();
void test_result_promise_get_result();

template<class type, class arguments_tuple_type>
void test_result_promise_set_value_impl(const arguments_tuple_type& tuple_args);
void test_result_promise_set_value();

template<class type>
void test_result_promise_set_exception_impl();
void test_result_promise_set_exception();

template<class type>
void test_result_promise_set_from_function_value_impl();
template<class type>
void test_result_promise_set_from_function_exception_impl();
void test_result_promise_set_from_function();

template<class type>
void test_rp_assignment_operator_impl_empty_to_empty();
template<class type>
void test_rp_assignment_operator_impl_non_empty_to_empty();
template<class type>
void test_rp_assignment_operator_impl_empty_to_non_empty();
template<class type>
void test_rp_assignment_operator_impl_non_empty_to_non_empty();
template<class type>
void test_rp_assignment_operator_assign_to_self();

template<class type>
void test_rp_assignment_operator_impl();
void test_rp_assignment_operator();
}  // namespace concurrencpp::tests

using concurrencpp::result_promise;
using concurrencpp::result;
using concurrencpp::result_status;

template<class type>
void concurrencpp::tests::test_result_promise_constructor_impl() {
    result_promise<type> rp;
    assert_true(static_cast<bool>(rp));

    auto other = std::move(rp);
    assert_true(static_cast<bool>(other));
    assert_false(static_cast<bool>(rp));
}

void concurrencpp::tests::test_result_promise_constructor() {
    test_result_promise_constructor_impl<int>();
    test_result_promise_constructor_impl<std::string>();
    test_result_promise_constructor_impl<void>();
    test_result_promise_constructor_impl<int&>();
    test_result_promise_constructor_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_promise_destructor_impl() {
    concurrencpp::result<type> result;

    {
        result_promise<type> rp;
        result = rp.get_result();
    }

    assert_true(static_cast<bool>(result));
    assert_equal(result.status(), result_status::exception);
    assert_throws<concurrencpp::errors::broken_task>([&] {
        result.get();
    });
}

void concurrencpp::tests::test_result_promise_RAII_impl() {
    object_observer observer;
    std::vector<result_promise<testing_stub>> rps;
    std::vector<result<testing_stub>> results;

    rps.resize(500'000);
    results.reserve(500'000);

    for (auto& rp : rps) {
        results.emplace_back(rp.get_result());
    }

    for (auto& rp : rps) {
        rp.set_result(observer.get_testing_stub());
    }

    rps.clear();
    assert_equal(observer.get_destruction_count(), size_t(0));

    results.clear();
    assert_equal(observer.get_destruction_count(), size_t(500'000));
}

void concurrencpp::tests::test_result_promise_destructor() {
    test_result_promise_destructor_impl<int>();
    test_result_promise_destructor_impl<std::string>();
    test_result_promise_destructor_impl<void>();
    test_result_promise_destructor_impl<int&>();
    test_result_promise_destructor_impl<std::string&>();

    test_result_promise_RAII_impl();
}

template<class type>
void concurrencpp::tests::test_result_promise_get_result_impl() {
    // trying to get the future more than once throws
    ::concurrencpp::result_promise<type> rp;
    auto result = rp.get_result();

    assert_true(static_cast<bool>(result));
    assert_equal(result.status(), concurrencpp::result_status::idle);

    assert_throws<concurrencpp::errors::result_already_retrieved>([&] {
        rp.get_result();
    });

    // empty rp should throw
    auto dummy = std::move(rp);
    assert_throws<concurrencpp::errors::empty_result_promise>([&] {
        rp.get_result();
    });
}

void concurrencpp::tests::test_result_promise_get_result() {
    test_result_promise_get_result_impl<int>();
    test_result_promise_get_result_impl<std::string>();
    test_result_promise_get_result_impl<void>();
    test_result_promise_get_result_impl<int&>();
    test_result_promise_get_result_impl<std::string&>();
}

template<class type, class arguments_tuple_type>
void concurrencpp::tests::test_result_promise_set_value_impl(
    const arguments_tuple_type& tuple_args) {
    auto set_rp = [](auto rp, auto tuple) {
        auto setter = [rp = std::move(rp)](auto&&... args) mutable {
            rp.set_result(args...);
        };

        std::apply(setter, tuple);
    };

    // basic test:
    {
        concurrencpp::result_promise<type> rp;
        auto result = rp.get_result();
        set_rp(std::move(rp), tuple_args);

        assert_false(static_cast<bool>(rp));
        test_ready_result_result(std::move(result));
    }

    // async test
    {
        concurrencpp::result_promise<type> rp;
        auto result = rp.get_result();

        std::thread thread([result = std::move(result)]() mutable {
            result.wait();
            test_ready_result_result(std::move(result));
        });

        set_rp(std::move(rp), tuple_args);
        assert_false(static_cast<bool>(rp));

        thread.join();
    }

    // if an exception is thrown inside set_value
    //(by building an object in place) the future is un-ready
    {
        struct throws_on_construction {
            throws_on_construction(int, int) {
                throw std::exception();
            }
        };

        concurrencpp::result_promise<throws_on_construction> rp;
        auto result = rp.get_result();

        assert_throws<std::exception>([&rp]() mutable {
            rp.set_result(0, 0);
        });

        assert_true(static_cast<bool>(result));
        assert_true(static_cast<bool>(rp));
        assert_equal(result.status(), result_status::idle);
    }

    // setting result to an empty rp throws
    {
        concurrencpp::result_promise<type> rp;
        auto dummy = std::move(rp);
        assert_throws<concurrencpp::errors::empty_result_promise>(
            [rp = std::move(rp), set_rp, tuple_args]() mutable {
                set_rp(std::move(rp), tuple_args);
            });
    }
}

void concurrencpp::tests::test_result_promise_set_value() {
    const auto expected_int_result = result_factory<int>::get();
    auto int_tuple = std::make_tuple(expected_int_result);
    test_result_promise_set_value_impl<int>(int_tuple);

    const auto expected_str_result = result_factory<std::string>::get();
    const auto modified_str_result =
        std::string("  ") + expected_str_result + " ";
    auto str_tuple =
        std::make_tuple(modified_str_result, 2, expected_str_result.size());
    test_result_promise_set_value_impl<std::string>(str_tuple);

    std::tuple<> empty_tuple;
    test_result_promise_set_value_impl<void>(empty_tuple);

    auto& expected_int_ref_result = result_factory<int&>::get();
    std::tuple<int&> int_ref_tuple = {expected_int_ref_result};
    test_result_promise_set_value_impl<int&>(int_ref_tuple);

    auto& expected_str_ref_result = result_factory<std::string&>::get();
    std::tuple<std::string&> str_ref_tuple = {expected_str_ref_result};
    test_result_promise_set_value_impl<std::string&>(str_ref_tuple);
}

template<class type>
void concurrencpp::tests::test_result_promise_set_exception_impl() {
    random randomizer;

    // basic test:
    {
        concurrencpp::result_promise<type> rp;
        auto result = rp.get_result();
        const auto id = randomizer();

        rp.set_exception(std::make_exception_ptr<costume_exception>(id));

        assert_false(static_cast<bool>(rp));
        test_ready_result_costume_exception(std::move(result), id);
    }

    // async test:
    {
        concurrencpp::result_promise<type> rp;
        auto result = rp.get_result();
        const auto id = randomizer();

        std::thread thread([result = std::move(result), id]() mutable {
            result.wait();
            test_ready_result_costume_exception(std::move(result), id);
        });

        rp.set_exception(std::make_exception_ptr<costume_exception>(id));
        assert_false(static_cast<bool>(rp));

        thread.join();
    }

    {  // can't set result to an empty rp
        result_promise<type> rp;
        auto dummy = std::move(rp);

        auto setter = [&rp] {
            rp.set_exception(std::make_exception_ptr(std::exception()));
        };

        assert_throws<concurrencpp::errors::empty_result_promise>(setter);
    }

    // null exception-ptr throws std::invalid_argument
    {
        result_promise<type> rp;

        auto setter = [&rp]() mutable {
            std::exception_ptr exception_ptr;
            rp.set_exception(exception_ptr);
        };

        assert_throws<std::invalid_argument>(setter);
    }
}

void concurrencpp::tests::test_result_promise_set_exception() {
    test_result_promise_set_exception_impl<int>();
    test_result_promise_set_exception_impl<std::string>();
    test_result_promise_set_exception_impl<void>();
    test_result_promise_set_exception_impl<int&>();
    test_result_promise_set_exception_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_promise_set_from_function_value_impl() {
    result_promise<type> rp;
    auto result = rp.get_result();
    rp.set_from_function(result_factory<type>::get);
    test_ready_result_result(std::move(result));
}

template<class type>
void concurrencpp::tests::
    test_result_promise_set_from_function_exception_impl() {
    random randomizer;
    result_promise<type> rp;
    auto result = rp.get_result();
    const auto id = randomizer();

    rp.set_from_function([id]() -> decltype(auto) {
        throw costume_exception(id);
        return result_factory<type>::get();
    });

    assert_false(static_cast<bool>(rp));
    assert_true(static_cast<bool>(result));
    assert_equal(result.status(), result_status::exception);
    test_ready_result_costume_exception(std::move(result), id);
}

void concurrencpp::tests::test_result_promise_set_from_function() {
    test_result_promise_set_from_function_value_impl<int>();
    test_result_promise_set_from_function_value_impl<std::string>();
    test_result_promise_set_from_function_value_impl<void>();
    test_result_promise_set_from_function_value_impl<int&>();
    test_result_promise_set_from_function_value_impl<std::string&>();

    test_result_promise_set_from_function_exception_impl<int>();
    test_result_promise_set_from_function_exception_impl<std::string&>();
    test_result_promise_set_from_function_exception_impl<void>();
    test_result_promise_set_from_function_exception_impl<int&>();
    test_result_promise_set_from_function_exception_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_rp_assignment_operator_impl_empty_to_empty() {
    result_promise<type> rp1, rp2;
    auto dummy1(std::move(rp1)), dummy2(std::move(rp2));

    assert_false(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));

    rp2 = std::move(rp1);

    assert_false(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));
}

template<class type>
void concurrencpp::tests::
    test_rp_assignment_operator_impl_non_empty_to_empty() {
    result_promise<type> rp1, rp2;
    auto dummy(std::move(rp2));

    assert_true(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));

    rp2 = std::move(rp1);

    assert_false(static_cast<bool>(rp1));
    assert_true(static_cast<bool>(rp2));
}

template<class type>
void concurrencpp::tests::
    test_rp_assignment_operator_impl_empty_to_non_empty() {
    result_promise<type> rp1, rp2;
    auto dummy(std::move(rp2));

    auto result = rp1.get_result();

    assert_true(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));

    rp1 = std::move(rp2);

    assert_false(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));

    assert_equal(result.status(), result_status::exception);
    assert_throws<concurrencpp::errors::broken_task>([&] {
        result.get();
    });
}

template<class type>
void concurrencpp::tests::
    test_rp_assignment_operator_impl_non_empty_to_non_empty() {
    result_promise<type> rp1, rp2;

    auto result1 = rp1.get_result();
    auto result2 = rp2.get_result();

    rp1 = std::move(rp2);

    assert_true(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));

    assert_equal(result1.status(), result_status::exception);
    assert_throws<concurrencpp::errors::broken_task>([&] {
        result1.get();
    });

    assert_equal(result2.status(), result_status::idle);
}

template<class type>
void concurrencpp::tests::test_rp_assignment_operator_assign_to_self() {
    result_promise<type> rp;
    rp = std::move(rp);
    assert_true(static_cast<bool>(rp));
}

template<class type>
void concurrencpp::tests::test_rp_assignment_operator_impl() {
    test_rp_assignment_operator_impl_empty_to_empty<type>();
    test_rp_assignment_operator_impl_non_empty_to_empty<type>();
    test_rp_assignment_operator_impl_empty_to_non_empty<type>();
    test_rp_assignment_operator_impl_non_empty_to_non_empty<type>();
    test_rp_assignment_operator_assign_to_self<type>();
}

void concurrencpp::tests::test_rp_assignment_operator() {
    test_rp_assignment_operator_impl<int>();
    test_rp_assignment_operator_impl<std::string>();
    test_rp_assignment_operator_impl<void>();
    test_rp_assignment_operator_impl<int&>();
    test_rp_assignment_operator_impl<std::string&>();
}

void concurrencpp::tests::test_result_promise() {
    tester tester("result_promise test");

    tester.add_step("constructor", test_result_promise_constructor);
    tester.add_step("destructor", test_result_promise_destructor);
    tester.add_step("get_result", test_result_promise_get_result);
    tester.add_step("set_value", test_result_promise_set_value);
    tester.add_step("set_exception", test_result_promise_set_exception);
    tester.add_step("set_from_function", test_result_promise_set_from_function);
    tester.add_step("operator =", test_rp_assignment_operator);

    tester.launch_test();
}
