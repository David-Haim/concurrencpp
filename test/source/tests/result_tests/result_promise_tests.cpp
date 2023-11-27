#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/test_ready_result.h"

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

    template<class type>
    void test_result_promise_set_value_impl();
    void test_result_promise_set_value_thrown_exception();
    void test_result_promise_set_value();

    template<class type>
    void test_result_promise_set_exception_impl();
    void test_result_promise_set_exception();

    template<class type>
    void test_result_promise_set_from_function_value_impl();
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

    template<class type>
    void set_result_promise(result_promise<type>& rp) {
        if constexpr (std::is_same_v<type, void>) {
            rp.set_result();
        } else {
            rp.set_result(value_gen<type>::default_value());
        }
    }
}  // namespace concurrencpp::tests

using concurrencpp::result;
using concurrencpp::result_status;
using concurrencpp::result_promise;
using concurrencpp::details::throw_helper;

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
    result<type> result;

    {
        result_promise<type> rp;
        result = rp.get_result();
    }

    assert_true(static_cast<bool>(result));
    assert_equal(result.status(), result_status::exception);
    assert_throws_with_error_message<concurrencpp::errors::broken_task>(
        [&] {
            result.get();
        },
        concurrencpp::details::consts::k_broken_task_exception_error_msg);
}

void concurrencpp::tests::test_result_promise_RAII_impl() {
    object_observer observer;

    {
        result<testing_stub> result;

        {
            result_promise<testing_stub> rp;
            result = rp.get_result();
            rp.set_result(observer.get_testing_stub());
        }

        assert_equal(observer.get_destruction_count(), static_cast<size_t>(0));
    }

    assert_equal(observer.get_destruction_count(), static_cast<size_t>(1));
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
    // empty result_promise should throw
    {
        const auto test_case = [] {
            result_promise<type> rp;
            auto dummy = std::move(rp);
            rp.get_result();
        };

        const auto expected_exception =
            throw_helper::make_empty_object_exception<errors::empty_result_promise>("result_promise", "get_result");

        assert_throws(test_case, expected_exception);
    }

    // valid result_promise returns a valid result
    result_promise<type> rp;
    auto result = rp.get_result();

    assert_true(static_cast<bool>(result));
    assert_equal(result.status(), concurrencpp::result_status::idle);

    // trying to get the future more than once throws
    assert_throws_with_error_message<concurrencpp::errors::result_already_retrieved>(
        [&] {
            rp.get_result();
        },
        concurrencpp::details::consts::k_result_promise_get_result_already_retrieved_error_msg);
}

void concurrencpp::tests::test_result_promise_get_result() {
    test_result_promise_get_result_impl<int>();
    test_result_promise_get_result_impl<std::string>();
    test_result_promise_get_result_impl<void>();
    test_result_promise_get_result_impl<int&>();
    test_result_promise_get_result_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_result_promise_set_value_impl() {
    // setting result to an empty rp throws
    {
        result_promise<type> rp;
        auto dummy = std::move(rp);

        const auto expected_exception =
            throw_helper::make_empty_object_exception<errors::empty_result_promise>(result_promise<int>::k_class_name, "set_result");

        assert_throws(
            [&rp] {
                set_result_promise(rp);
            },
            expected_exception);
    }

    // setting a (valid) result passes it to the associated result object and empties the result_promise
    {
        result_promise<type> rp;
        auto result = rp.get_result();

        std::thread thread([result = std::move(result)]() mutable {
            result.wait();
            test_ready_result(std::move(result));
        });

        set_result_promise(rp);
        assert_false(static_cast<bool>(rp));

        thread.join();
    }
}

void concurrencpp::tests::test_result_promise_set_value_thrown_exception() {
    // this test checks that even after an exception was thrown while constructing a value, the rp is idle and can be set again
    result_promise<std::string> rp;
    auto result = rp.get_result();

    assert_throws<std::length_error>([&] {
        std::string str;
        rp.set_result(str.max_size() + 10, 'a');
    });

    assert_equal(result.status(), result_status::idle);
    assert_true(static_cast<bool>(rp));

    const char* str = "hello world";
    rp.set_result(str);  // should not throw.
    assert_equal(result.status(), result_status::value);
    assert_equal(result.get(), str);
}

void concurrencpp::tests::test_result_promise_set_value() {
    test_result_promise_set_value_impl<int>();
    test_result_promise_set_value_impl<std::string>();
    test_result_promise_set_value_impl<void>();
    test_result_promise_set_value_impl<int&>();
    test_result_promise_set_value_impl<std::string&>();

    test_result_promise_set_value_thrown_exception();
}

template<class type>
void concurrencpp::tests::test_result_promise_set_exception_impl() {
    // can't set result to an empty rp
    {
        result_promise<type> rp;
        auto dummy = std::move(rp);

        const auto expected_exception =
            throw_helper::make_empty_object_exception<errors::empty_result_promise>(result_promise<int>::k_class_name,
                                                                                    "set_exception");

        assert_throws(
            [&rp] {
                rp.set_exception(std::make_exception_ptr(std::exception()));
            },
            expected_exception);
    }

    // null exception_ptr throws std::invalid_argument
    {
        const auto expected_exception =
            throw_helper::make_empty_argument_exception(result_promise<int>::k_class_name, "set_exception", "exception_ptr");

        assert_throws(
            [] {
                result_promise<type> rp;
                rp.set_exception(std::exception_ptr {});
            },
            expected_exception);
    }

    // basic test:
    {
        concurrencpp::result_promise<type> rp;
        auto result = rp.get_result();
        const auto id = 123456789;

        std::thread thread([result = std::move(result), id]() mutable {
            result.wait();
            test_ready_result_custom_exception(std::move(result), id);
        });

        rp.set_exception(std::make_exception_ptr<custom_exception>(id));
        assert_false(static_cast<bool>(rp));

        thread.join();
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
    // empty rp throws
    {
        result_promise<type> rp;
        auto dummy = std::move(rp);

        const auto expected_exception =
            throw_helper::make_empty_object_exception<errors::empty_result_promise>(result_promise<int>::k_class_name,
                                                                                    "set_from_function");
        assert_throws(
            [&rp] {
                rp.set_from_function(value_gen<type>::default_value);
            },
            expected_exception);
    }

    // setting a valid value
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        rp.set_from_function(value_gen<type>::default_value);
        test_ready_result(std::move(result));
    }

    // function throws an exception
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        const auto id = 123456789;

        rp.set_from_function([id]() -> decltype(auto) {
            throw custom_exception(id);
            return value_gen<type>::default_value();
        });

        assert_false(static_cast<bool>(rp));
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), result_status::exception);
        test_ready_result_custom_exception(std::move(result), id);
    }
}

void concurrencpp::tests::test_result_promise_set_from_function() {
    test_result_promise_set_from_function_value_impl<int>();
    test_result_promise_set_from_function_value_impl<std::string>();
    test_result_promise_set_from_function_value_impl<void>();
    test_result_promise_set_from_function_value_impl<int&>();
    test_result_promise_set_from_function_value_impl<std::string&>();
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
void concurrencpp::tests::test_rp_assignment_operator_impl_non_empty_to_empty() {
    result_promise<type> rp1, rp2;
    auto dummy(std::move(rp2));

    assert_true(static_cast<bool>(rp1));
    assert_false(static_cast<bool>(rp2));

    rp2 = std::move(rp1);

    assert_false(static_cast<bool>(rp1));
    assert_true(static_cast<bool>(rp2));
}

template<class type>
void concurrencpp::tests::test_rp_assignment_operator_impl_empty_to_non_empty() {
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
void concurrencpp::tests::test_rp_assignment_operator_impl_non_empty_to_non_empty() {
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

using namespace concurrencpp::tests;

int main() {
    tester tester("result_promise test");

    tester.add_step("constructor", test_result_promise_constructor);
    tester.add_step("destructor", test_result_promise_destructor);
    tester.add_step("get_result", test_result_promise_get_result);
    tester.add_step("set_value", test_result_promise_set_value);
    tester.add_step("set_exception", test_result_promise_set_exception);
    tester.add_step("set_from_function", test_result_promise_set_from_function);
    tester.add_step("operator =", test_rp_assignment_operator);

    tester.launch_test();
    return 0;
}