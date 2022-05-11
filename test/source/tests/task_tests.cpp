#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"

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
    void test_task_assignment_operator_to_self();
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
            const char buffer[concurrencpp::details::task_constants::buffer_size + 1] = {};

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

template<class promise_type>
using coroutine_handle = concurrencpp::details::coroutine_handle<promise_type>;
using suspend_always = concurrencpp::details::suspend_always;
using suspend_never = concurrencpp::details::suspend_never;

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
        std::unique_ptr<int> up;

       public:
        functor_with_unique_ptr() : up(std::make_unique<int>(12345)) {}
        functor_with_unique_ptr(functor_with_unique_ptr&& rhs) noexcept = default;

        int operator()() noexcept {
            assert(static_cast<bool>(up));
            return *up;
        }
    };

    struct trivially_copiable_destructable {
        int operator()() const {
            return 123456789;
        }
    };

    static_assert(std::is_trivially_copy_constructible_v<trivially_copiable_destructable>);
    static_assert(std::is_trivially_destructible_v<trivially_copiable_destructable>);

    struct test_promise {
        coroutine_handle<void> get_return_object() noexcept {
            return coroutine_handle<test_promise>::from_promise(*this);
        }

        suspend_always initial_suspend() const noexcept {
            return {};
        }

        suspend_never final_suspend() const noexcept {
            return {};
        }

        void unhandled_exception() const noexcept {}

        void return_void() const noexcept {}
    };

    struct dummy_test_tag {};

}  // namespace concurrencpp::tests::functions

namespace CRCPP_COROUTINE_NAMESPACE {
    template<class... arguments>
    struct coroutine_traits<coroutine_handle<>, functions::dummy_test_tag, arguments...> {
        using promise_type = functions::test_promise;
    };

}  // namespace CRCPP_COROUTINE_NAMESPACE

namespace concurrencpp::tests::functions {
    coroutine_handle<void> coro_function(dummy_test_tag) {
        co_return;
    }

    coroutine_handle<void> coro_function(dummy_test_tag, testing_stub stub) {
        stub();
        co_return;
    }
}  // namespace concurrencpp::tests::functions

using namespace concurrencpp::tests::functions;
using concurrencpp::task;

void concurrencpp::tests::test_task_constructor() {
    // Default constructor:
    {
        task task;
        assert_false(static_cast<bool>(task));
    }

    // Function pointer
    {
        int (*function_ptr)() = &g_test_function;
        task task(function_ptr);
        assert_true(static_cast<bool>(task));
        assert_true(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<100>>());
        assert_false(task.contains<::coroutine_handle<void>>());
    }

    // Function reference
    {
        int (&function_reference)() = g_test_function;
        task task(function_reference);
        assert_true(static_cast<bool>(task));
        assert_true(task.contains<int (&)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<100>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // small functor
    {
        test_functor<8> small_functor;
        task task(small_functor);
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_true(task.contains<test_functor<8>>());
        assert_false(task.contains<test_functor<100>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // small lambda
    {
        std::array<char, 8> padding_s;
        auto lambda = [padding_s]() mutable {
            padding_s.fill(0);
        };

        task task(lambda);
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<8>>());
        assert_false(task.contains<test_functor<100>>());
        assert_true(task.contains<decltype(lambda)>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // big functor
    {
        test_functor<128> big_functor;
        task task(big_functor);
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_true(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // big lambda
    {

        std::array<char, 128> padding_b;
        auto big_lambda = [padding_b]() mutable {
            padding_b.fill(0);
        };

        task task(big_lambda);
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_true(task.contains<decltype(big_lambda)>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // Coroutine handle
    {
        auto handle = coro_function({});
        task task_from_coroutine_handle(handle);
        assert_true(static_cast<bool>(task_from_coroutine_handle));
        assert_false(task_from_coroutine_handle.contains<int (*)()>());
        assert_false(task_from_coroutine_handle.contains<test_functor<1>>());
        assert_false(task_from_coroutine_handle.contains<test_functor<128>>());
        assert_true(task_from_coroutine_handle.contains<coroutine_handle<void>>());
    }

    // A move only type
    {
        functor_with_unique_ptr fwup;
        task task(std::move(fwup));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
        assert_true(task.contains<functor_with_unique_ptr>());
    }
}

void concurrencpp::tests::test_task_move_constructor() {
    // Default constructor:
    {
        task base_task;
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_false(static_cast<bool>(task));
    }

    // Function pointer
    {
        int (*function_pointer)() = &g_test_function;
        task base_task(function_pointer);
        task task(std::move(base_task));

        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_true(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // Function reference
    {
        int (&function_reference)() = g_test_function;
        task base_task(function_reference);
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_true(task.contains<int (&)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // small functor
    {
        test_functor<8> small_functor;
        task base_task(small_functor);
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_true(task.contains<test_functor<8>>());
        assert_false(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // small lambda
    {
        std::array<char, 8> padding_s;
        auto small_lambda = [padding_s]() mutable {
            padding_s.fill(0);
        };

        task base_task(small_lambda);
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<8>>());
        assert_false(task.contains<test_functor<128>>());
        assert_true(task.contains<decltype(small_lambda)>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // Big functor
    {
        test_functor<128> big_functor;
        task base_task(big_functor);
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<8>>());
        assert_true(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // Big lambda
    {
        std::array<char, 128> padding_b;
        auto big_lambda = [padding_b]() mutable {
            padding_b.fill(0);
        };

        task base_task(big_lambda);
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<8>>());
        assert_false(task.contains<test_functor<128>>());
        assert_true(task.contains<decltype(big_lambda)>());
        assert_false(task.contains<coroutine_handle<void>>());
    }

    // Coroutine handle
    {
        auto handle = coro_function({});
        task base_task(handle);
        task task(std::move(base_task));
        assert_false(static_cast<bool>(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_true(task.contains<coroutine_handle<void>>());
    }

    // A move only type
    {
        functor_with_unique_ptr fwup;
        task base_task(std::move(fwup));
        task task(std::move(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
        assert_true(task.contains<functor_with_unique_ptr>());
    }

    // trivially copiable (uses memcpy)
    {
        trivially_copiable_destructable tcd;
        task base_task(tcd);
        task task(std::move(base_task));
        assert_true(static_cast<bool>(task));
        assert_false(task.contains<int (*)()>());
        assert_false(task.contains<test_functor<1>>());
        assert_false(task.contains<test_functor<128>>());
        assert_false(task.contains<coroutine_handle<void>>());
        assert_false(task.contains<functor_with_unique_ptr>());
        assert_true(task.contains<trivially_copiable_destructable>());
    }
}

void concurrencpp::tests::test_task_destructor() {
    // empty task
    { task empty; }

    // small functor
    {
        object_observer observer;

        { task task(observer.get_testing_stub()); }

        assert_equal(observer.get_destruction_count(), 1);
    }

    // big functor
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

    // coroutine
    {
        object_observer observer;
        const auto handle = coro_function({}, observer.get_testing_stub());

        { task t(handle); }

        assert_equal(observer.get_destruction_count(), 1);
    }

    // trivially destructible (does nothing)
    {
        trivially_copiable_destructable tcd;
        task task(tcd);
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

    // coroutine
    {
        object_observer observer;
        const auto handle = functions::coro_function({}, observer.get_testing_stub());
        task t(handle);
        t.clear();
        assert_false(static_cast<bool>(t));
        assert_equal(observer.get_destruction_count(), 1);
    }

    // trivially destructible (does nothing)
    {
        trivially_copiable_destructable tcd;
        task t(tcd);
        t.clear();
        assert_false(static_cast<bool>(t));
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
    // empty task
    task t;
    t();
}

void concurrencpp::tests::test_task_call_operator_test_5() {
    // a croutine
    object_observer observer;
    const auto handle = coro_function({}, observer.get_testing_stub());

    assert_equal(observer.get_execution_count(), 0);
    assert_equal(observer.get_destruction_count(), 0);

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
    task t1, t2;
    t1 = std::move(t2);

    assert_false(static_cast<bool>(t1));
    assert_false(static_cast<bool>(t2));
}

void concurrencpp::tests::test_task_assignment_operator_empty_to_non_empty() {
    object_observer observer;
    task t1(observer.get_testing_stub()), t2;
    t1 = std::move(t2);

    assert_false(static_cast<bool>(t1));
    assert_false(static_cast<bool>(t2));
    assert_equal(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_non_empty_to_empty() {
    object_observer observer;
    task t1, t2(observer.get_testing_stub());
    t1 = std::move(t2);

    assert_true(static_cast<bool>(t1));
    assert_false(static_cast<bool>(t2));

    t1();

    assert_equal(observer.get_destruction_count(), 1);
    assert_equal(observer.get_execution_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_non_empty_to_non_empty() {
    object_observer observer_1, observer_2;

    task t1(observer_1.get_testing_stub()), t2(observer_2.get_testing_stub());
    t1 = std::move(t2);

    assert_true(static_cast<bool>(t1));
    assert_false(static_cast<bool>(t2));

    t1();

    assert_equal(observer_1.get_destruction_count(), 1);
    assert_equal(observer_1.get_execution_count(), 0);

    assert_equal(observer_2.get_destruction_count(), 1);
    assert_equal(observer_2.get_execution_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_to_self() {
    object_observer observer;
    task task;

    task = std::move(task);
    assert_false(static_cast<bool>(task));

    task = concurrencpp::task {observer.get_testing_stub()};
    task = std::move(task);
    assert_true(static_cast<bool>(task));
    assert_true(task.contains<testing_stub>());
}

void concurrencpp::tests::test_task_assignment_operator() {
    test_task_assignment_operator_empty_to_empty();
    test_task_assignment_operator_empty_to_non_empty();
    test_task_assignment_operator_non_empty_to_empty();
    test_task_assignment_operator_non_empty_to_non_empty();
    test_task_assignment_operator_to_self();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("task test");

    tester.add_step("constructor", test_task_constructor);
    tester.add_step("move constructor", test_task_move_constructor);
    tester.add_step("destructor", test_task_destructor);
    tester.add_step("operator()", test_task_call_operator);
    tester.add_step("clear", test_task_clear);
    tester.add_step("operator =", test_task_assignment_operator);

    tester.launch_test();
    return 0;
}