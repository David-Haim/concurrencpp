#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"

using namespace concurrencpp::tests;

namespace concurrencpp::tests {
    void test_generator_move_constructor();
    void test_generator_destructor();
    void test_generator_begin();

    template<class type>
    void test_generator_begin_end_impl();
    void test_generator_begin_end();

    void test_generator_iterator_operator_plus_plus_exception();
    void test_generator_iterator_operator_plus_plus_ran_to_end();
    void test_generator_iterator_operator_plus_plus();

    void test_generator_iterator_operator_plus_plus_postfix_exception();
    void test_generator_iterator_operator_plus_plus_postfix_ran_to_end();
    void test_generator_iterator_operator_plus_plus_postfix();

    void test_generator_iterator_dereferencing_operators();
    void test_generator_iterator_comparison_operators();
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_generator_move_constructor() {
    auto gen0 = []() -> generator<int> {
        co_yield 1;
    }();

    assert_true(static_cast<bool>(gen0));

    generator<int> gen1(std::move(gen0));
    assert_false(static_cast<bool>(gen0));
    assert_true(static_cast<bool>(gen1));
}

void concurrencpp::tests::test_generator_destructor() {
    auto gen_fn = [](testing_stub stub) -> generator<int> {
        co_yield 1;
    };

    object_observer observer;

    {
        auto gen0 = gen_fn(observer.get_testing_stub());
        auto gen1(std::move(gen0));  // check to see that empty generator d.tor is benign
    }

    assert_equal(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_generator_begin() {
    auto gen0 = []() -> generator<int> {
        throw custom_exception(1234567);
        co_yield 1;
    }();

    assert_throws<custom_exception>([&gen0] {
        gen0.begin();
    });

    auto gen1 = []() -> generator<int> {
        co_yield 1;
    }();

    auto gen2(std::move(gen1));
    assert_throws_with_error_message<errors::empty_generator>(
        [&gen1] {
            gen1.begin();
        },
        concurrencpp::details::consts::k_empty_generator_begin_err_msg);
}

template<class type>
void concurrencpp::tests::test_generator_begin_end_impl() {
    value_gen<type> val_gen;
    auto gen = [&val_gen]() -> generator<type> {
        for (size_t i = 0; i < 1024; i++) {
            co_yield val_gen.value_of(i);
        }
    };

    size_t counter = 0;
    for (const auto& val : gen()) {
        if constexpr (std::is_reference_v<type>) {
            assert_equal(&val, &val_gen.value_of(counter));
        } else {
            assert_equal(val, val_gen.value_of(counter));
        }
        counter++;
    }

    assert_equal(counter, 1024);
}

void concurrencpp::tests::test_generator_begin_end() {
    test_generator_begin_end_impl<int>();
    test_generator_begin_end_impl<std::string>();
    test_generator_begin_end_impl<int&>();
    test_generator_begin_end_impl<std::string&>();
}

void concurrencpp::tests::test_generator_iterator_operator_plus_plus_exception() {
    auto gen = []() -> generator<int> {
        for (auto i = 0; i < 10; i++) {
            if (i != 0 && i % 3 == 0) {
                throw custom_exception(i);
            }

            co_yield i;
        }
    }();

    auto gen_it = gen.begin();  // i = 0
    const auto end = gen.end();
    assert_not_equal(gen_it, end);

    auto& res0 = ++gen_it;  // i = 1
    assert_equal(&res0, &gen_it);
    assert_not_equal(gen_it, end);

    auto& res1 = ++gen_it;  // i = 2
    assert_equal(&res1, &gen_it);
    assert_not_equal(gen_it, end);

    assert_throws<custom_exception>([&gen_it] {
        ++gen_it;
    });  // i = 3
    assert_equal(gen_it, end);
}

void concurrencpp::tests::test_generator_iterator_operator_plus_plus_ran_to_end() {
    auto gen = []() -> generator<int> {
        for (auto i = 0; i < 3; i++) {
            co_yield i;
        }
    }();

    auto gen_it = gen.begin();  // i = 0
    const auto end = gen.end();
    assert_not_equal(gen_it, end);

    auto& res0 = ++gen_it;  // i = 1
    assert_equal(&res0, &gen_it);
    assert_not_equal(gen_it, end);

    auto& res1 = ++gen_it;  // i = 2
    assert_equal(&res1, &gen_it);
    assert_not_equal(gen_it, end);

    auto& res2 = ++gen_it;  // i = 3
    assert_equal(&res2, &gen_it);
    assert_equal(gen_it, end);
}

void concurrencpp::tests::test_generator_iterator_operator_plus_plus() {
    test_generator_iterator_operator_plus_plus_exception();
    test_generator_iterator_operator_plus_plus_ran_to_end();
}

void concurrencpp::tests::test_generator_iterator_operator_plus_plus_postfix_exception() {
    auto gen = []() -> generator<int> {
        for (auto i = 0; i < 10; i++) {
            if (i != 0 && i % 3 == 0) {
                throw custom_exception(i);
            }

            co_yield i;
        }
    }();

    auto gen_it = gen.begin();  // i = 0
    const auto end = gen.end();
    assert_not_equal(gen_it, end);

    gen_it++;  // i = 1
    assert_not_equal(gen_it, end);

    gen_it++;  // i = 2
    assert_not_equal(gen_it, end);

    assert_throws<custom_exception>([&gen_it] {
        gen_it++;
    });  // i = 3

    assert_equal(gen_it, end);
}

void concurrencpp::tests::test_generator_iterator_operator_plus_plus_postfix_ran_to_end() {
    auto gen = []() -> generator<int> {
        for (auto i = 0; i < 3; i++) {
            co_yield i;
        }
    }();

    auto gen_it = gen.begin();  // i = 0
    const auto end = gen.end();
    assert_not_equal(gen_it, end);

    gen_it++;  // i = 1
    assert_not_equal(gen_it, end);

    gen_it++;  // i = 2
    assert_not_equal(gen_it, end);

    gen_it++;  // i = 3
    assert_equal(gen_it, end);
}

void concurrencpp::tests::test_generator_iterator_operator_plus_plus_postfix() {
    test_generator_iterator_operator_plus_plus_postfix_exception();
    test_generator_iterator_operator_plus_plus_postfix_ran_to_end();
}

void concurrencpp::tests::test_generator_iterator_dereferencing_operators() {
    struct dummy {
        int i = 0;
    };

    dummy arr[6] = {};
    auto gen = [](std::span<dummy> s) -> generator<dummy> {
        int i = 0;
        while (true) {
            co_yield s[i % s.size()];
            ++i;
        }
    }(arr);

    auto it = gen.begin();
    for (size_t i = 0; i < 100; i++) {
        auto& ref = *it;
        auto* ptr = &it->i;

        auto& expected = arr[i % std::size(arr)];
        assert_equal(&ref, &expected);
        assert_equal(ptr, &expected.i);
        ++it;
    }
}

void concurrencpp::tests::test_generator_iterator_comparison_operators() {
    auto gen = []() -> generator<int> {
        co_yield 1;
        co_yield 2;
    };

    auto gen0 = gen();
    auto gen1 = gen();

    const auto it0 = gen0.begin();
    const auto it1 = gen1.begin();
    const auto copy = it0;
    const auto end = gen0.end();

    // operator ==
    {
        assert_true(it0 == it0);
        assert_true(it0 == copy);
        assert_true(copy == it0);
        assert_false(it0 == it1);
        assert_false(it1 == it0);
        assert_false(it0 == end);
        assert_false(end == it0);
    }

    // operator !=
    {
        assert_false(it0 != it0);
        assert_false(it0 != copy);
        assert_false(copy != it0);
        assert_true(it0 != it1);
        assert_true(it1 != it0);
        assert_true(it0 != end);
        assert_true(end != it0);
    }
}

int main() {
    {
        tester tester("generator test");

        tester.add_step("move constructor", test_generator_move_constructor);
        tester.add_step("destructor", test_generator_destructor);
        tester.add_step("begin", test_generator_begin);
        tester.add_step("begin + end", test_generator_begin_end);

        tester.launch_test();
    }

    {
        tester tester("generator_iterator test");

        tester.add_step("operator ++it", test_generator_iterator_operator_plus_plus);
        tester.add_step("operator it++", test_generator_iterator_operator_plus_plus_postfix);
        tester.add_step("operator *, operator ->", test_generator_iterator_dereferencing_operators);
        tester.add_step("operator ==, operator !=", test_generator_iterator_comparison_operators);

        tester.launch_test();
    }

    return 0;
}
