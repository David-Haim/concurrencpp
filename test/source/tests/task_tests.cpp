#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/object_observer.h"

#include <array>

using namespace concurrencpp::tests;

namespace concurrencpp::tests {
    void test_task_constructor();
    void test_task_move_constructor();

    void test_task_destructor();

    void test_task_clear();

    void test_task_call_operator_test_1();
    void test_task_call_operator_test_2();
    void test_task_call_operator_test_3();
    void test_task_call_operator_test_4();
    void test_task_call_operator_test_5();
    void test_task_call_operator();

    void test_task_assignment_operator_empty_to_empty();
    void test_task_assignment_operator_empty_to_non_empty();
    void test_task_assignment_operator_non_empty_to_empty();
    void test_task_assignment_operator_non_empty_to_non_empty();
    void test_task_assignment_operator();

}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    struct callable_inlining_tester {
        struct not_movable {
            not_movable(const not_movable&) {}

            void operator()() {}
        };

        struct not_noexcept_movable {
            not_noexcept_movable(not_noexcept_movable&&) {}

            void operator()() {}
        };

        struct noexcept_movable_big {
            const char buffer[1024 * 4] = {};

            noexcept_movable_big(noexcept_movable_big&&) noexcept {}

            void operator()() {}
        };

        struct inlinable {
            char buffer[concurrencpp::details::task_constants::buffer_size];

            inlinable(inlinable&&) noexcept {}

            void operator()() {}
        };

        static_assert(!concurrencpp::details::callable_vtable<not_movable>::is_inlinable(),
                      "callable_vtable<...>::is_inlinable - callable_vtable deduced un-inlinable functor is inlinable.");

        static_assert(!concurrencpp::details::callable_vtable<not_noexcept_movable>::is_inlinable(),
                      "callable_vtable<...>::is_inlinable - callable_vtable deduced un-inlinable functor is inlinable.");

        static_assert(!concurrencpp::details::callable_vtable<noexcept_movable_big>::is_inlinable(),
                      "callable_vtable<...>::is_inlinable - callable_vtable deduced un-inlinable functor is inlinable.");

        static_assert(concurrencpp::details::callable_vtable<inlinable>::is_inlinable(),
                      "callable_vtable<...>::is_inlinable - callable_vtable deduced inlinable functor is *NOT* inlinable.");
    };
}  // namespace concurrencpp::tests

namespace concurrencpp::tests::functions {
    template<size_t N>
    class test_functor {

       private:
        char m_buffer[N] = {};

       public:
        int operator()() const {
            return 123456789;
        }
    };

    int g_test_function() {
        return 123456789;
    }

    class functor_with_unique_ptr {
       private:
        std::unique_ptr<int> m_up;

       public:
        functor_with_unique_ptr() : m_up(std::make_unique<int>()) {}
        functor_with_unique_ptr(functor_with_unique_ptr&& rhs) noexcept = default;

        void operator()() {}
    };

    struct test_promise {
        std::experimental::coroutine_handle<> get_return_object() noexcept {
            return std::experimental::coroutine_handle<test_promise>::from_promise(*this);
        }

        std::experimental::suspend_always initial_suspend() const noexcept {
            return {};
        }

        std::experimental::suspend_never final_suspend() const noexcept {
            return {};
        }

        void unhandled_exception() const noexcept {}

        void return_void() const noexcept {}
    };

    struct dummy_test_tag {};

}  // namespace concurrencpp::tests::functions

namespace std::experimental {
    template<class... arguments>
    struct coroutine_traits<std::experimental::coroutine_handle<>, functions::dummy_test_tag, arguments...> {
        using promise_type = functions::test_promise;
    };

}  // namespace std::experimental

namespace concurrencpp::tests::functions {
    std::experimental::coroutine_handle<> coro_function(dummy_test_tag) {
        co_return;
    }

    std::experimental::coroutine_handle<> coro_function(dummy_test_tag, testing_stub stub) {
        stub();
        co_return;
    }
}  // namespace concurrencpp::tests::functions

using concurrencpp::task;
using namespace concurrencpp::tests::functions;

using coroutine_handle = std::experimental::coroutine_handle<>;

void concurrencpp::tests::test_task_constructor() {
    // Default constructor:
    task empty_task;
    assert_false(static_cast<bool>(empty_task));

    // Function pointer
    int (*function_ptr)() = &g_test_function;
    task task_from_function_pointer(function_ptr);
    assert_true(static_cast<bool>(task_from_function_pointer));
    assert_true(task_from_function_pointer.contains<int (*)()>());
    assert_false(task_from_function_pointer.contains<test_functor<1>>());
    assert_false(task_from_function_pointer.contains<test_functor<100>>());
    assert_false(task_from_function_pointer.contains<coroutine_handle>());

    // Function reference
    int (&function_reference)() = g_test_function;
    task task_from_function_reference(function_reference);
    assert_true(static_cast<bool>(task_from_function_reference));
    assert_true(task_from_function_reference.contains<int (*)()>());
    assert_false(task_from_function_reference.contains<test_functor<1>>());
    assert_false(task_from_function_reference.contains<test_functor<100>>());
    assert_false(task_from_function_reference.contains<coroutine_handle>());

    // small functor
    test_functor<8> small_functor;
    task task_from_small_functor(small_functor);
    assert_true(static_cast<bool>(task_from_small_functor));
    assert_false(task_from_small_functor.contains<int (*)()>());
    assert_true(task_from_small_functor.contains<test_functor<8>>());
    assert_false(task_from_small_functor.contains<test_functor<100>>());
    assert_false(task_from_small_functor.contains<coroutine_handle>());

    // small lambda
    std::array<char, 8> padding_s;
    auto lambda = [padding_s]() mutable {
        padding_s.fill(0);
    };

    task task_from_small_lambda(lambda);
    assert_true(static_cast<bool>(task_from_small_lambda));
    assert_false(task_from_small_lambda.contains<int (*)()>());
    assert_false(task_from_small_lambda.contains<test_functor<8>>());
    assert_false(task_from_small_lambda.contains<test_functor<100>>());
    assert_true(task_from_small_lambda.contains<decltype(lambda)>());
    assert_false(task_from_small_lambda.contains<coroutine_handle>());

    // big functor
    test_functor<128> big_functor;
    task task_from_big_functor(big_functor);
    assert_true(static_cast<bool>(task_from_big_functor));
    assert_false(task_from_big_functor.contains<int (*)()>());
    assert_false(task_from_big_functor.contains<test_functor<1>>());
    assert_true(task_from_big_functor.contains<test_functor<128>>());
    assert_false(task_from_big_functor.contains<coroutine_handle>());

    // big lambda
    std::array<char, 128> padding_b;
    auto big_lambda = [padding_b]() mutable {
        padding_b.fill(0);
    };

    task task_from_big_lambda(big_lambda);
    assert_true(static_cast<bool>(task_from_big_lambda));
    assert_false(task_from_big_lambda.contains<int (*)()>());
    assert_false(task_from_big_lambda.contains<test_functor<1>>());
    assert_false(task_from_big_lambda.contains<test_functor<128>>());
    assert_true(task_from_big_lambda.contains<decltype(big_lambda)>());
    assert_false(task_from_big_lambda.contains<coroutine_handle>());

    // Coroutine handle
    auto handle = coro_function({});
    task task_from_coroutine_handle(handle);
    assert_true(static_cast<bool>(task_from_coroutine_handle));
    assert_false(task_from_coroutine_handle.contains<int (*)()>());
    assert_false(task_from_coroutine_handle.contains<test_functor<1>>());
    assert_false(task_from_coroutine_handle.contains<test_functor<128>>());
    assert_false(task_from_coroutine_handle.contains<decltype(big_lambda)>());
    assert_true(task_from_coroutine_handle.contains<coroutine_handle>());

    // A move only type
    functor_with_unique_ptr fwup;
    task move_only_functor(std::move(fwup));
    assert_true(static_cast<bool>(move_only_functor));
    assert_false(move_only_functor.contains<int (*)()>());
    assert_false(move_only_functor.contains<test_functor<1>>());
    assert_false(move_only_functor.contains<test_functor<128>>());
    assert_false(move_only_functor.contains<decltype(big_lambda)>());
    assert_false(move_only_functor.contains<coroutine_handle>());
    assert_false(move_only_functor.contains<functor_with_unique_ptr>());
}

void concurrencpp::tests::test_task_move_constructor() {
    // Default constructor:
    task base_task_0;
    task empty_task(std::move(base_task_0));
    assert_false(static_cast<bool>(base_task_0));
    assert_false(static_cast<bool>(empty_task));

    // Function pointer
    int (*task_pointer)() = &g_test_function;
    task base_task_2(task_pointer);
    task task_from_function_pointer(std::move(base_task_2));

    assert_false(static_cast<bool>(base_task_2));
    assert_true(static_cast<bool>(task_from_function_pointer));
    assert_true(task_from_function_pointer.contains<int (*)()>());
    assert_false(task_from_function_pointer.contains<test_functor<1>>());
    assert_false(task_from_function_pointer.contains<test_functor<128>>());
    assert_false(task_from_function_pointer.contains<coroutine_handle>());

    // Function reference
    int (&function_reference)() = g_test_function;
    task base_task_3(function_reference);
    task task_from_function_reference(std::move(base_task_3));

    assert_false(static_cast<bool>(base_task_3));
    assert_true(static_cast<bool>(task_from_function_reference));
    assert_true(task_from_function_reference.contains<int (&)()>());
    assert_false(task_from_function_reference.contains<test_functor<1>>());
    assert_false(task_from_function_reference.contains<test_functor<128>>());
    assert_false(task_from_function_reference.contains<coroutine_handle>());

    // small functor
    test_functor<8> small_functor;
    task base_task_4(small_functor);
    task task_from_small_functor(std::move(base_task_4));

    assert_false(static_cast<bool>(base_task_4));
    assert_true(static_cast<bool>(task_from_small_functor));
    assert_false(task_from_small_functor.contains<int (*)()>());
    assert_true(task_from_small_functor.contains<test_functor<8>>());
    assert_false(task_from_small_functor.contains<test_functor<128>>());
    assert_false(task_from_small_functor.contains<coroutine_handle>());

    // small lambda
    std::array<char, 8> padding_s;
    auto small_lambda = [padding_s]() mutable {
        padding_s.fill(0);
    };

    task base_task_5(small_lambda);
    task task_from_small_lambda(std::move(base_task_5));

    assert_false(static_cast<bool>(base_task_5));
    assert_true(static_cast<bool>(task_from_small_lambda));
    assert_false(task_from_small_lambda.contains<int (*)()>());
    assert_false(task_from_small_lambda.contains<test_functor<8>>());
    assert_false(task_from_small_lambda.contains<test_functor<128>>());
    assert_true(task_from_small_lambda.contains<decltype(small_lambda)>());
    assert_false(task_from_small_lambda.contains<coroutine_handle>());

    // Big functor
    test_functor<128> big_functor;
    task base_task_6(big_functor);
    task task_from_big_functor(std::move(base_task_6));

    assert_false(static_cast<bool>(base_task_6));
    assert_true(static_cast<bool>(task_from_big_functor));
    assert_false(task_from_big_functor.contains<int (*)()>());
    assert_false(task_from_big_functor.contains<test_functor<8>>());
    assert_true(task_from_big_functor.contains<test_functor<128>>());
    assert_false(task_from_big_functor.contains<coroutine_handle>());

    // Big lambda
    std::array<char, 128> padding_b;
    auto big_lambda = [padding_b]() mutable {
        padding_b.fill(0);
    };

    task base_task_7(big_lambda);
    task task_from_big_lambda(std::move(base_task_7));

    assert_false(static_cast<bool>(base_task_7));
    assert_true(static_cast<bool>(task_from_big_lambda));
    assert_false(task_from_big_lambda.contains<int (*)()>());
    assert_false(task_from_big_lambda.contains<test_functor<8>>());
    assert_false(task_from_big_lambda.contains<test_functor<128>>());
    assert_true(task_from_big_lambda.contains<decltype(big_lambda)>());
    assert_false(task_from_big_lambda.contains<coroutine_handle>());

    // Coroutine handle
    auto handle = coro_function({});
    task base_task_8(handle);
    task task_from_coroutine_handle(std::move(base_task_8));

    assert_false(static_cast<bool>(base_task_8));
    assert_true(static_cast<bool>(task_from_coroutine_handle));
    assert_false(task_from_coroutine_handle.contains<int (*)()>());
    assert_false(task_from_coroutine_handle.contains<test_functor<1>>());
    assert_false(task_from_coroutine_handle.contains<test_functor<128>>());
    assert_false(task_from_coroutine_handle.contains<decltype(big_lambda)>());
    assert_true(task_from_coroutine_handle.contains<coroutine_handle>());

    // A move only type
    functor_with_unique_ptr fwup;
    task base_task_9(std::move(fwup));
    task move_only_functor(std::move(fwup));
    assert_true(static_cast<bool>(move_only_functor));
    assert_false(move_only_functor.contains<int (*)()>());
    assert_false(move_only_functor.contains<test_functor<1>>());
    assert_false(move_only_functor.contains<test_functor<128>>());
    assert_false(move_only_functor.contains<decltype(big_lambda)>());
    assert_false(move_only_functor.contains<coroutine_handle>());
    assert_false(move_only_functor.contains<functor_with_unique_ptr>());
}

void concurrencpp::tests::test_task_destructor() {
    // empty function
    { task empty; }

    // small function
    {
        object_observer observer;

        { task task(observer.get_testing_stub()); }

        assert_equal(observer.get_destruction_count(), 1);
    }

    // big function
    {
        struct functor {
            testing_stub stub;
            char padding[1024] = {};

            functor(testing_stub stub) noexcept : stub(std::move(stub)) {}

            void operator()() {}
        };

        object_observer observer;

        { task t(functor {observer.get_testing_stub()}); }

        assert_equal(observer.get_destruction_count(), 1);
    }

    {
        object_observer observer;
        const auto handle = coro_function({}, observer.get_testing_stub());

        { task t(handle); }

        assert_equal(observer.get_destruction_count(), 1);
    }
}

void concurrencpp::tests::test_task_clear() {
    // empty function
    {
        task empty;
        empty.clear();
        assert_false(static_cast<bool>(empty));
    }

    // small function
    {
        object_observer observer;
        task t(observer.get_testing_stub());
        t.clear();
        assert_false(static_cast<bool>(t));
        assert_equal(observer.get_destruction_count(), 1);
    }

    // big function
    {
        struct functor {
            testing_stub stub;
            char padding[1024] = {};

            functor(testing_stub stub) noexcept : stub(std::move(stub)) {}

            void operator()() {}
        };

        object_observer observer;
        task t(functor {observer.get_testing_stub()});
        t.clear();
        assert_false(static_cast<bool>(t));
        assert_equal(observer.get_destruction_count(), 1);
    }

    {
        object_observer observer;
        const auto handle = functions::coro_function({}, observer.get_testing_stub());
        task t(handle);
        t.clear();
        assert_equal(observer.get_destruction_count(), 1);
    }
}

void concurrencpp::tests::test_task_call_operator_test_1() {
    object_observer observer;
    task t(observer.get_testing_stub());

    t();

    assert_equal(observer.get_execution_count(), 1);
    assert_equal(observer.get_destruction_count(), 1);
    assert_false(static_cast<bool>(t));
}

void concurrencpp::tests::test_task_call_operator_test_2() {
    task t([] {
        throw std::logic_error("example");
    });

    assert_throws<std::logic_error>(t);
}

void concurrencpp::tests::test_task_call_operator_test_3() {
    bool a = false, b = false, c = false;
    int d = 1234567890, e = 0;
    auto lambda = [](bool& _a, bool* _b, bool _c, int d, int& e) mutable {
        _a = true;
        *_b = true;
        _c = true;
        e = d;
    };

    task t(concurrencpp::details::bind(lambda, std::ref(a), &b, c, d, std::ref(e)));
    t();

    assert_true(a);
    assert_true(b);
    assert_false(c);
    assert_equal(e, d);
}

void concurrencpp::tests::test_task_call_operator_test_4() {
    task t;
    t();
}

void concurrencpp::tests::test_task_call_operator_test_5() {
    object_observer observer;
    const auto handle = coro_function({}, observer.get_testing_stub());
    task t(handle);
    t();

    assert_equal(observer.get_execution_count(), 1);
    assert_equal(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_task_call_operator() {
    test_task_call_operator_test_1();
    test_task_call_operator_test_2();
    test_task_call_operator_test_3();
    test_task_call_operator_test_4();
    test_task_call_operator_test_5();
}

void concurrencpp::tests::test_task_assignment_operator_empty_to_empty() {
    task f1, f2;
    f1 = std::move(f2);

    assert_false(static_cast<bool>(f1));
    assert_false(static_cast<bool>(f2));
}

void concurrencpp::tests::test_task_assignment_operator_empty_to_non_empty() {
    object_observer observer;
    task f1(observer.get_testing_stub()), f2;
    f1 = std::move(f2);

    assert_false(static_cast<bool>(f1));
    assert_false(static_cast<bool>(f2));
    assert_equal(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_non_empty_to_empty() {
    object_observer observer;
    task f1, f2(observer.get_testing_stub());
    f1 = std::move(f2);

    assert_true(static_cast<bool>(f1));
    assert_false(static_cast<bool>(f2));

    f1();

    assert_equal(observer.get_destruction_count(), 1);
    assert_equal(observer.get_execution_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_non_empty_to_non_empty() {
    object_observer observer_1, observer_2;

    task f1(observer_1.get_testing_stub()), f2(observer_2.get_testing_stub());
    f1 = std::move(f2);

    assert_true(static_cast<bool>(f1));
    assert_false(static_cast<bool>(f2));

    f1();

    assert_equal(observer_1.get_destruction_count(), 1);
    assert_equal(observer_1.get_execution_count(), 0);

    assert_equal(observer_2.get_destruction_count(), 1);
    assert_equal(observer_2.get_execution_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator() {
    test_task_assignment_operator_empty_to_empty();
    test_task_assignment_operator_empty_to_non_empty();
    test_task_assignment_operator_non_empty_to_empty();
    test_task_assignment_operator_non_empty_to_non_empty();
}

void concurrencpp::tests::test_task() {
    tester tester("task test");

    tester.add_step("task constructor", test_task_constructor);
    tester.add_step("task move constructor", test_task_move_constructor);
    tester.add_step("task destructor", test_task_destructor);
    tester.add_step("task operator()", test_task_call_operator);
    tester.add_step("task clear", test_task_clear);
    tester.add_step("task operator =", test_task_assignment_operator);

    tester.launch_test();
}
