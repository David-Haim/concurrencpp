#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/object_observer.h"

#include <array>
#include <atomic>

using namespace concurrencpp::tests;

namespace concurrencpp::tests {
	void test_task_constructor();
	void test_task_move_constructor();

	void test_task_destructor();

	void test_task_clear_RAII();
	void test_task_clear_functionality();
	void test_task_clear();

	void test_task_call_operator_test_1();
	void test_task_call_operator_test_2();
	void test_task_call_operator_test_3();
	void test_task_call_operator_test_4();
	void test_task_call_operator();

	void test_task_cancel();

	void test_task_assignment_operator_empty_to_empty();
	void test_task_assignment_operator_empty_to_non_empty();
	void test_task_assignment_operator_non_empty_to_empty();
	void test_task_assignment_operator_non_empty_to_non_empty();
	void test_task_assignment_operator();
}

namespace concurrencpp::tests::functions {
	template<size_t N>
	class test_functor {

	private:
		char m_buffer[N];

	public:
		int operator()() { return 123456789; }
	};

	int g_test_function() { return 123456789; }
}

namespace concurrencpp::tests {
	struct cancel_existence_tester {

		struct no_cancel_1 {};

		struct no_cancel_2 {
			void cancel_(std::exception_ptr) noexcept {}
		};

		struct cancel_not_void {
			int cancel(std::exception_ptr) noexcept {}
		};

		struct cancel_not_ex_ptr {
			void cancel(const std::exception&) noexcept {}
		};

		struct cancel_not_noexcept {
			void cancel(std::exception_ptr) {}
		};

		struct contains_cancel {
			void cancel(std::exception_ptr) noexcept {}
		};

		static_assert(
			!concurrencpp::details::functor_traits<no_cancel_1>::has_cancel(),
			"functor_traits<...>::has_cancel - found cancel in a class with no valid cancel method.");

		static_assert(
			!concurrencpp::details::functor_traits<no_cancel_2>::has_cancel(),
			"functor_traits<...>::has_cancel - found cancel in a class with no valid cancel method.");

		static_assert(
			!concurrencpp::details::functor_traits<cancel_not_void>::has_cancel(),
			"functor_traits<...>::has_cancel - found cancel in a class with no valid cancel method.");

		static_assert(
			!concurrencpp::details::functor_traits<cancel_not_ex_ptr>::has_cancel(),
			"functor_traits<...>::has_cancel - found cancel in a class with no valid cancel method.");

		static_assert(
			!concurrencpp::details::functor_traits<cancel_not_noexcept>::has_cancel(),
			"functor_traits<...>::has_cancel - found cancel in a class with no valid cancel method.");

		static_assert(
			concurrencpp::details::functor_traits<contains_cancel>::has_cancel(),
			"functor_traits<...>::has_cancel - did *NOT* found cancel in a class with a valid cancel method.");
	};

	struct buffer_inlining_tester {

		struct not_movable {
			not_movable(not_movable&&) = delete;
		};

		struct not_noexcept_movable {
			not_noexcept_movable(not_noexcept_movable&&) {}
		};

		struct noexcept_movable_big {

			char buffer[1024 * 4];

			noexcept_movable_big(noexcept_movable_big&&) noexcept {}
		};

		struct inlinable {

			char buffer[concurrencpp::details::task_traits::k_inline_buffer_size];

			inlinable(inlinable&&) noexcept {}
		};

		static_assert(
			!concurrencpp::details::functor_traits<not_movable>::is_inlinable,
			"functor_traits<...>::is_inlinable - function_traits deduced un-inlinable functor is inlinable.");

		static_assert(
			!concurrencpp::details::functor_traits<not_noexcept_movable>::is_inlinable,
			"functor_traits<...>::is_inlinable - function_traits deduced un-inlinable functor is inlinable.");

		static_assert(
			!concurrencpp::details::functor_traits<noexcept_movable_big>::is_inlinable,
			"functor_traits<...>::is_inlinable - function_traits deduced un-inlinable functor is inlinable.");

		static_assert(
			concurrencpp::details::functor_traits<inlinable>::is_inlinable,
			"functor_traits<...>::is_inlinable - function_traits deduced inlinable functor is *NOT* inlinable.");
	};
}

using concurrencpp::task;
using namespace concurrencpp::tests::functions;

void concurrencpp::tests::test_task_constructor() {
	//default constructor:
	task empty_task;
	assert_false(static_cast<bool>(empty_task));
	assert_true(empty_task.uses_sbo());
	assert_same(empty_task.type(), typeid(std::nullptr_t));

	//nullptr constructor
	task null_task(nullptr);
	assert_false(static_cast<bool>(null_task));
	assert_true(null_task.uses_sbo());
	assert_same(empty_task.type(), typeid(std::nullptr_t));

	//Function pointer
	int(*task_pointer)() = &g_test_function;
	task task_from_function_pointer(task_pointer);
	assert_true(static_cast<bool>(task_from_function_pointer));
	assert_true(task_from_function_pointer.uses_sbo());
	assert_same(task_from_function_pointer.type(), typeid(int(*)()));
	assert_not_same(task_from_function_pointer.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<1>));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<100>));

	//Function reference
	int(&function_reference)() = g_test_function;
	task task_from_function_reference(function_reference);
	assert_true(static_cast<bool>(task_from_function_reference));
	assert_true(task_from_function_reference.uses_sbo());
	assert_same(task_from_function_pointer.type(), typeid(int(*)()));
	assert_not_same(task_from_function_pointer.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<1>));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<100>));

	//small functor
	test_functor<8> small_functor;
	task task_from_small_functor(small_functor);
	assert_true(static_cast<bool>(task_from_small_functor));
	assert_true(task_from_small_functor.uses_sbo());
	assert_not_same(task_from_small_functor.type(), typeid(int(*)()));
	assert_not_same(task_from_small_functor.type(), typeid(std::nullptr_t));
	assert_same(task_from_small_functor.type(), typeid(test_functor<8>));
	assert_not_same(task_from_small_functor.type(), typeid(test_functor<100>));

	//small lambda
	std::array<char, 8> padding_s;
	auto lambda = [padding_s]() mutable {
		padding_s.fill(0);
		return 0;
	};

	task task_from_small_lambda(lambda);
	assert_true(static_cast<bool>(task_from_small_lambda));
	assert_true(task_from_small_lambda.uses_sbo());
	assert_not_same(task_from_small_lambda.type(), typeid(int(*)()));
	assert_not_same(task_from_small_lambda.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_small_lambda.type(), typeid(test_functor<8>));
	assert_not_same(task_from_small_lambda.type(), typeid(test_functor<100>));
	assert_same(task_from_small_lambda.type(), typeid(decltype(lambda)));

	//big functor
	test_functor<128> big_functor;
	task task_from_big_functor(big_functor);
	assert_true(static_cast<bool>(task_from_big_functor));
	assert_false(task_from_big_functor.uses_sbo());
	assert_not_same(task_from_big_functor.type(), typeid(int(*)()));
	assert_not_same(task_from_big_functor.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_big_functor.type(), typeid(test_functor<8>));
	assert_same(task_from_big_functor.type(), typeid(test_functor<128>));

	//big lambda
	std::array<char, 128> padding_b;
	auto big_lambda = [padding_b]() mutable {
		padding_b.fill(0);
		return 0;
	};

	task task_from_big_lambda(big_lambda);
	assert_true(static_cast<bool>(task_from_big_lambda));
	assert_not_same(task_from_big_lambda.type(), typeid(int(*)(int)));
	assert_not_same(task_from_big_lambda.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_big_lambda.type(), typeid(test_functor<8>));
	assert_not_same(task_from_big_lambda.type(), typeid(test_functor<128>));
	assert_same(task_from_big_lambda.type(), typeid(decltype(big_lambda)));
}

void concurrencpp::tests::test_task_move_constructor() {
	//default constructor:
	task base_task_0;
	task empty_task(std::move(base_task_0));

	assert_false(static_cast<bool>(base_task_0));
	assert_false(static_cast<bool>(empty_task));
	assert_true(empty_task.uses_sbo());
	assert_same(empty_task.type(), typeid(std::nullptr_t));

	//nullptr constructor
	task base_task_1(nullptr);
	task null_task(std::move(base_task_1));

	assert_false(static_cast<bool>(base_task_1));
	assert_false(static_cast<bool>(null_task));
	assert_true(null_task.uses_sbo());
	assert_same(empty_task.type(), typeid(std::nullptr_t));

	//Function pointer
	int(*task_pointer)() = &g_test_function;
	task base_task_2(task_pointer);
	task task_from_function_pointer(std::move(base_task_2));
	
	assert_false(static_cast<bool>(base_task_2));
	assert_true(static_cast<bool>(task_from_function_pointer));
	assert_true(task_from_function_pointer.uses_sbo());
	assert_same(task_from_function_pointer.type(), typeid(int(*)()));
	assert_not_same(task_from_function_pointer.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<1>));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<100>));

	//Function reference
	int(&function_reference)() = g_test_function;
	task base_task_3(task_pointer);
	task task_from_function_reference(std::move(base_task_3));

	assert_false(static_cast<bool>(base_task_3));
	assert_true(static_cast<bool>(task_from_function_reference));
	assert_true(task_from_function_reference.uses_sbo());
	assert_same(task_from_function_pointer.type(), typeid(int(*)()));
	assert_not_same(task_from_function_pointer.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<1>));
	assert_not_same(task_from_function_pointer.type(), typeid(test_functor<100>));

	//small functor
	test_functor<8> small_functor;
	task base_task_4(small_functor);
	task task_from_small_functor(std::move(base_task_4));
	
	assert_false(static_cast<bool>(base_task_4));
	assert_true(static_cast<bool>(task_from_small_functor));
	assert_true(task_from_small_functor.uses_sbo());
	assert_not_same(task_from_small_functor.type(), typeid(int(*)()));
	assert_not_same(task_from_small_functor.type(), typeid(std::nullptr_t));
	assert_same(task_from_small_functor.type(), typeid(test_functor<8>));
	assert_not_same(task_from_small_functor.type(), typeid(test_functor<100>));

	//small lambda
	std::array<char, 8> padding_s;
	auto lambda = [padding_s]() mutable {
		padding_s.fill(0);
		return 0;
	};

	task base_task_5(lambda);
	task task_from_small_lambda(std::move(base_task_5));

	assert_false(static_cast<bool>(base_task_5));
	assert_true(static_cast<bool>(task_from_small_lambda));
	assert_true(task_from_small_lambda.uses_sbo());
	assert_not_same(task_from_small_lambda.type(), typeid(int(*)()));
	assert_not_same(task_from_small_lambda.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_small_lambda.type(), typeid(test_functor<8>));
	assert_not_same(task_from_small_lambda.type(), typeid(test_functor<100>));
	assert_same(task_from_small_lambda.type(), typeid(decltype(lambda)));

	//big functor
	test_functor<128> big_functor;
	task base_task_6(big_functor);
	task task_from_big_functor(std::move(base_task_6));

	assert_false(static_cast<bool>(base_task_6));
	assert_true(static_cast<bool>(task_from_big_functor));
	assert_false(task_from_big_functor.uses_sbo());
	assert_not_same(task_from_big_functor.type(), typeid(int(*)()));
	assert_not_same(task_from_big_functor.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_big_functor.type(), typeid(test_functor<8>));
	assert_same(task_from_big_functor.type(), typeid(test_functor<128>));

	//big lambda
	std::array<char, 128> padding_b;
	auto big_lambda = [padding_b]() mutable {
		padding_b.fill(0);
		return 0;
	};

	task base_task_7(big_lambda);
	task task_from_big_lambda(std::move(base_task_7));

	assert_false(static_cast<bool>(base_task_7));
	assert_true(static_cast<bool>(task_from_big_lambda));
	assert_not_same(task_from_big_lambda.type(), typeid(int(*)(int)));
	assert_not_same(task_from_big_lambda.type(), typeid(std::nullptr_t));
	assert_not_same(task_from_big_lambda.type(), typeid(test_functor<8>));
	assert_not_same(task_from_big_lambda.type(), typeid(test_functor<128>));
	assert_same(task_from_big_lambda.type(), typeid(decltype(big_lambda)));

}

void concurrencpp::tests::test_task_destructor() {
	//empty function
	{
		task empty;
		task empty_2(nullptr);
	}

	//small function
	{
		object_observer observer;

		{
			task t(observer.get_testing_stub());
		}

		assert_same(observer.get_destruction_count(), 1);
	}

	//big function
	{
		object_observer observer;
		std::array<char, 128> padding;
		auto big_lambda = [padding, stub = observer.get_testing_stub()]{};

		{
			task t(std::move(big_lambda));
		}

		assert_same(observer.get_destruction_count(), 1);
	}
}

void concurrencpp::tests::test_task_clear_RAII() {
	//empty function
	{
		task empty;
		empty.clear();
		assert_false(static_cast<bool>(empty));
	}

	//small function
	{
		object_observer observer;
		task t(observer.get_testing_stub());
		t.clear();
		assert_false(static_cast<bool>(t));
		assert_same(observer.get_destruction_count(), 1);
	}

	//big function
	{
		object_observer observer;
		std::array<char, 128> padding;
		auto big_lambda = [padding, stub = observer.get_testing_stub()]{};
		task t(std::move(big_lambda));
		t.clear();
		assert_false(static_cast<bool>(t));
		assert_same(observer.get_destruction_count(), 1);
	}
}

void concurrencpp::tests::test_task_clear_functionality() {
	task t;

	t.clear();
	assert_false(static_cast<bool>(t));

	t = g_test_function;
	assert_true(t);

	assert_same(t.type(), typeid(&g_test_function));

	t.clear();
	assert_false(static_cast<bool>(t));

	test_functor<8> small_functor;
	t = small_functor;
	assert_true(t);
	assert_same(t.type(), typeid(test_functor<8>));

	t.clear();
	assert_false(static_cast<bool>(t));

	std::array<char, 8> small_padding;
	auto small_lambda = [small_padding]() mutable { return 0;  };
	t = small_lambda;
	assert_true(t);
	assert_same(t.type(), typeid(decltype(small_lambda)));

	t.clear();
	assert_false(static_cast<bool>(t));

	test_functor<128> big_functor;
	t = big_functor;
	assert_true(t);
	assert_same(t.type(), typeid(test_functor<128>));

	t.clear();
	assert_false(static_cast<bool>(t));

	std::array<char, 128> big_padding;
	auto big_lambda = [big_padding]() mutable { return 0;  };
	t = big_lambda;
	assert_true(t);
	assert_same(t.type(), typeid(decltype(big_lambda)));

	t.clear();
	assert_false(static_cast<bool>(t));
}

void concurrencpp::tests::test_task_clear() {
	test_task_clear_RAII();
	test_task_clear_functionality();
}

void concurrencpp::tests::test_task_call_operator_test_1() {
	object_observer observer;
	task t(observer.get_testing_stub(std::chrono::milliseconds(0)));

	for (auto i = 0; i < 1'000; i++) {
		t();
	}

	assert_same(observer.get_execution_count(), 1'000);
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

	task t(concurrencpp::details::bind(lambda, std::ref(a), &b, c,  d, std::ref(e)));
	t();

	assert_true(a);
	assert_true(b);
	assert_false(c);
	assert_same(e, d);
}

void concurrencpp::tests::test_task_call_operator_test_4() {
	task t;
	t();
}

void concurrencpp::tests::test_task_call_operator() {
	test_task_call_operator_test_1();
	test_task_call_operator_test_2();
	test_task_call_operator_test_3();
	test_task_call_operator_test_4();
}

void concurrencpp::tests::test_task_cancel() {
	auto exception_ptr = std::make_exception_ptr(std::runtime_error("error"));

	//empty task
	{
		task t;
		t.cancel(exception_ptr);
		assert_false(static_cast<bool>(t));
	}

	//functor with no cancel
	{
		struct no_cancel {
			void operator()() noexcept {}
		};

		task t(no_cancel{});
		assert_true(static_cast<bool>(t));

		t.cancel(exception_ptr);
		assert_false(static_cast<bool>(t));
	}

	//functor with cancel
	{
		class functor {

		private:
			std::exception_ptr& m_exception;

		public:
			functor(std::exception_ptr& exception) noexcept : m_exception(exception) {}

			void operator()() noexcept {}

			void cancel(std::exception_ptr e) noexcept {
				m_exception = e;
			}
		};

		std::exception_ptr expected_exception;
		task t = functor{ expected_exception };

		assert_true(static_cast<bool>(t));

		t.cancel(exception_ptr);
		assert_false(static_cast<bool>(t));

		assert_same(expected_exception, exception_ptr);
	}
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
	assert_same(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_non_empty_to_empty() {
	object_observer observer;
	task f1, f2(observer.get_testing_stub());
	f1 = std::move(f2);

	assert_true(static_cast<bool>(f1));
	assert_false(static_cast<bool>(f2));

	f1();

	assert_same(observer.get_destruction_count(), 0);
	assert_same(observer.get_execution_count(), 1);
}

void concurrencpp::tests::test_task_assignment_operator_non_empty_to_non_empty() {
	object_observer observer_1, observer_2;

	task f1(observer_1.get_testing_stub()), f2(observer_2.get_testing_stub());
	f1 = std::move(f2);

	assert_true(static_cast<bool>(f1));
	assert_false(static_cast<bool>(f2));

	f1();

	assert_same(observer_1.get_destruction_count(), 1);
	assert_same(observer_1.get_execution_count(), 0);

	assert_same(observer_2.get_destruction_count(), 0);
	assert_same(observer_2.get_execution_count(), 1);
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
	tester.add_step("task cancel", test_task_cancel);
	tester.add_step("task clear", test_task_clear);
	tester.add_step("task operator =", test_task_assignment_operator);

	tester.launch_test();
}
