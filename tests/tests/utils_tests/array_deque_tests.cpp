#include "concurrencpp.h"
#include "../all_tests.h"

#include "../../tester/tester.h"
#include "../../helpers/assertions.h"
#include "../../helpers/fast_randomizer.h"
#include "../../helpers/object_observer.h"
#include "../../helpers/result_generator.h"

#include <tuple>
#include <deque>

#include "../../concurrencpp/src/utils/array_deque.h"

using namespace concurrencpp::tests;
using concurrencpp::details::array_deque;

namespace concurrencpp::tests {

	template<class type>
	void test_array_deque_default_constructor_impl();
	void test_array_deque_default_constructor();

	template<class type>
	void test_array_deque_move_constructor_impl();
	void test_array_deque_move_constructor();

	void test_array_deque_destructor();
	void test_array_deque_clear();

	template<class type>
	void test_array_deque_emplace_back_impl();
	void test_array_deque_emplace_back();

	template<class type>
	void test_array_deque_pop_back_impl();
	void test_array_deque_pop_back();

	template<class type>
	void test_array_deque_emplace_front_impl();
	void test_array_deque_emplace_front();

	template<class type>
	void test_array_deque_pop_front_impl();
	void test_array_deque_pop_front();

	template<class type>
	void test_array_deque_equal_not_equal_impl();
	void test_array_deque_equal_not_equal();

	template<class type>
	void test_array_deque_combo_impl();
	void test_array_deque_combo();
}

namespace concurrencpp::tests {
	template<class type>
	bool deque_equal(const array_deque<type>& array_deque, const std::deque<type>& deque) {
		return (array_deque.size() == deque.size()) &&
			std::equal(array_deque.begin(), array_deque.end(), deque.begin());
	}
}

template<class type>
void concurrencpp::tests::test_array_deque_default_constructor_impl() {
	array_deque<type> deque;

	assert_same(deque.size(), 0);
	assert_same(deque.capacity(), 0);
	assert_true(deque.empty());
	assert_same(deque.begin(), deque.end());
}

void concurrencpp::tests::test_array_deque_default_constructor() {
	test_array_deque_default_constructor_impl<int>();
	test_array_deque_default_constructor_impl<std::string>();
}

template<class type>
void concurrencpp::tests::test_array_deque_move_constructor_impl() {
	array_deque<type> deque_0;
	const auto& c_deque_0 = deque_0;
	std::deque<type> std_deque;
	result_generator<type> gen;

	for (auto i = 0; i < 1'000; i++) {
		const auto& value = gen();
		deque_0.emplace_back(value);
		std_deque.emplace_back(value);
	}

	array_deque<type> deque_1(std::move(deque_0));
	const auto& c_deque_1 = deque_1;

	assert_same(deque_0.size(), 0);
	assert_true(deque_0.empty());
	assert_same(deque_0.capacity(), 0);
	assert_same(deque_0.begin(), deque_0.end());
	assert_same(c_deque_0.begin(), c_deque_0.end());

	assert_same(deque_1.size(), 1'000);
	assert_false(deque_1.empty());
	assert_bigger(deque_1.capacity() , 0);
	assert_not_same(deque_1.begin(), deque_1.end());
	assert_not_same(c_deque_1.begin(), c_deque_1.end());

	assert_true(deque_equal(deque_1, std_deque));
}

void concurrencpp::tests::test_array_deque_move_constructor() {
	test_array_deque_move_constructor_impl<int>();
	test_array_deque_move_constructor_impl<std::string>();
}

void concurrencpp::tests::test_array_deque_destructor() {
	object_observer observer;

	{
		array_deque<object_observer::testing_stub> deque;

		for (size_t i = 0; i < 1'000; i++) {
			deque.emplace_front(observer.get_testing_stub());
		}

		for (size_t i = 0; i < 1'000; i++) {
			deque.emplace_back(observer.get_testing_stub());
		}

		for (size_t i = 0; i < 950; i++) {
			deque.pop_back();
		}

		for (size_t i = 0; i < 950; i++) {
			deque.pop_front();
		}

		for (size_t i = 0; i < 1'000; i++) {
			deque.emplace_back(observer.get_testing_stub());
		}

		for (size_t i = 0; i < 1'000; i++) {
			deque.emplace_front(observer.get_testing_stub());
		}

		for (size_t i = 0; i < 345; i++) {
			deque.pop_back();
		}

		for (size_t i = 0; i < 678; i++) {
			deque.pop_front();
		}

		//the rest of the elements are destroyed here by the destructor
	}

	assert_same(observer.get_destruction_count(), 4'000);
}

void concurrencpp::tests::test_array_deque_clear() {
	object_observer observer;
	array_deque<object_observer::testing_stub> deque;

	for (size_t i = 0; i < 1'000; i++) {
		deque.emplace_front(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 1'000; i++) {
		deque.emplace_back(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 950; i++) {
		deque.pop_back();
	}

	for (size_t i = 0; i < 950; i++) {
		deque.pop_front();
	}

	for (size_t i = 0; i < 1'000; i++) {
		deque.emplace_back(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 1'000; i++) {
		deque.emplace_front(observer.get_testing_stub());
	}

	for (size_t i = 0; i < 345; i++) {
		deque.pop_back();
	}

	for (size_t i = 0; i < 678; i++) {
		deque.pop_front();
	}

	deque.clear();
	assert_same(observer.get_destruction_count(), 4'000);
}

template<class type>
void concurrencpp::tests::test_array_deque_emplace_back_impl() {
	array_deque<type> arr_deque;
	std::deque<type> std_deque;
	result_generator<type> gen;

	for (size_t i = 0; i < 1024 * 16; i++) {
		const auto& value = gen();
		arr_deque.emplace_back(value);
		std_deque.emplace_back(value);

		assert_bigger_equal(arr_deque.capacity() , arr_deque.size());
		assert_true(deque_equal(arr_deque, std_deque));
	}
}

void concurrencpp::tests::test_array_deque_emplace_back() {
	test_array_deque_emplace_back_impl<int>();
	test_array_deque_emplace_back_impl<std::string>();
}

template<class type>
void concurrencpp::tests::test_array_deque_pop_back_impl() {
	array_deque<type> arr_deque;
	std::deque<type> std_deque;
	result_generator<type> gen;

	for (size_t i = 0; i < 1024 * 16; i++) {
		const auto& value = gen();
		arr_deque.emplace_back(value);
		std_deque.emplace_back(value);
	}

	for (size_t i = 0; i < 1024 * 16; i++) {
		auto value0 = arr_deque.pop_back();
		auto value1 = std::move(std_deque.back());
		std_deque.pop_back();

		assert_same(value0, value1);
		assert_bigger_equal(arr_deque.capacity() , arr_deque.size());
		assert_true(deque_equal(arr_deque, std_deque));
	}
}

void concurrencpp::tests::test_array_deque_pop_back() {
	test_array_deque_pop_back_impl<int>();
	test_array_deque_pop_back_impl<std::string>();
}

template<class type>
void concurrencpp::tests::test_array_deque_emplace_front_impl() {
	array_deque<type> arr_deque;
	std::deque<type> std_deque;
	result_generator<type> gen;

	for (size_t i = 0; i < 1024 * 16; i++) {
		const auto& value = gen();
		arr_deque.emplace_front(value);
		std_deque.emplace_front(value);

		assert_bigger_equal(arr_deque.capacity() , arr_deque.size());
		assert_true(deque_equal(arr_deque, std_deque));
	}
}

void concurrencpp::tests::test_array_deque_emplace_front() {
	test_array_deque_emplace_front_impl<int>();
	test_array_deque_emplace_front_impl<std::string>();
}

template<class type>
void concurrencpp::tests::test_array_deque_pop_front_impl() {
	array_deque<type> arr_deque;
	std::deque<type> std_deque;
	result_generator<type> gen;

	for (size_t i = 0; i < 1024 * 16; i++) {
		const auto& value = gen();
		arr_deque.emplace_front(value);
		std_deque.emplace_front(value);
	}

	for (size_t i = 0; i < 1024 * 16; i++) {
		auto value0 = arr_deque.pop_front();
		auto value1 = std::move(std_deque.front());
		std_deque.pop_front();

		assert_same(value0, value1);
		assert_bigger_equal(arr_deque.capacity() , arr_deque.size());
		assert_true(deque_equal(arr_deque, std_deque));
	}
}

void concurrencpp::tests::test_array_deque_pop_front() {
	test_array_deque_pop_front_impl<int>();
	test_array_deque_pop_front_impl<std::string>();
}

template<class type>
void concurrencpp::tests::test_array_deque_equal_not_equal_impl() {
	result_generator<type> gen;

	//a deque is always equal to itself
	{
		array_deque<type> deque;

		for (auto i = 0; i < 500; i++) {
			assert_true(deque == deque);
			deque.emplace_front(gen());
		}
	}

	//empty == empty
	{
		array_deque<type> deque0, deque1;
		assert_true(deque0 == deque1);
		assert_false(deque0 != deque1);
	}

	//empty != non empty
	{
		array_deque<type> deque0, deque1;
		deque1.emplace_back(gen());
		assert_true(deque0 != deque1);
		assert_false(deque0 == deque1);
	}

	//non empty != empty
	{
		array_deque<type> deque0, deque1;
		deque0.emplace_back(gen());
		assert_true(deque0 != deque1);
		assert_false(deque0 == deque1);
	}

	//deques are equal. note : we try to make 2 deques with 2 different capacities
	{
		array_deque<type> deque0, deque1;
		for (auto i = 0; i < 500; i++) {
			deque0.emplace_front(gen());
		}

		for (auto i = 0; i < 1'000; i++) {
			const auto& value = gen();
			deque0.emplace_back(value);
			deque1.emplace_back(value);
		}

		for (auto i = 0; i < 500; i++) {
			deque0.pop_front();
		}

		assert_not_same(deque0.capacity(), deque1.capacity());
		assert_same(deque0.size(), deque1.size());

		for (auto i = 0; i < 1'000; i++) {
			assert_true(deque0 == deque1);
			assert_false(deque0 != deque1);
			deque0.pop_back();
			deque1.pop_back();
		}
	}

	//sizes are equal but deques are not
	{
		array_deque<type> deque0, deque1;

		for (auto i = 0; i < 1'000; i++) {
			const auto& value = gen();
			deque0.emplace_back(value);
			deque1.emplace_back(value);
		}

		const auto& value0 = gen();
		const auto& value1 = gen();
		assert_not_same(value0, value1);
		deque0.emplace_back(value0);
		deque1.emplace_back(value1);

		assert_false(deque0 == deque1);
		assert_true(deque0 != deque1);

		deque0.pop_back();
		deque1.pop_back();

		assert_false(deque0 != deque1);
		assert_true(deque0 == deque1);
	}
}

void concurrencpp::tests::test_array_deque_equal_not_equal() {
	test_array_deque_equal_not_equal_impl<int>();
	test_array_deque_equal_not_equal_impl<std::string>();
}

template<class type>
void concurrencpp::tests::test_array_deque_combo_impl() {
	enum class end {
		front,
		back
	};

	enum class action {
		add,
		remove
	};

	std::vector<std::tuple<end, action, size_t>> steps = {
		{ end::front, action::add, 123 },
		{ end::back, action::add, 234 },
		{ end::front, action::add, 456 },
		{ end::front, action::remove, 289 },
		{ end::back, action::remove, 311 },
		{ end::back, action::add, 1327 },
		{ end::front, action::add, 100 },
		{ end::back, action::remove, 745 },
		{ end::front, action::remove, 13 },
		{ end::front, action::add, 1578 },
		{ end::back, action::add, 327 },
		{ end::front, action::add, 98 },
		{ end::back, action::remove, 5 },
		{ end::front, action::remove, 27 },
		{ end::front, action::add, 2035 },
		{ end::back, action::add, 12 },
		{ end::front, action::add, 45 },
		{ end::back, action::remove, 50 },
		{ end::front, action::remove, 211 },
		{ end::front, action::add, 100 },
	};

	std::deque<type> std_deque;
	array_deque<type> arr_deque;
	result_generator<type> gen;

	auto cb = [&](end e, action a, size_t count) {
		if (a == action::add) {
			if (e == end::back) {
				for (size_t i = 0; i < count; i++) {
					const auto& value = gen();
					std_deque.emplace_back(value);
					arr_deque.emplace_back(value);
				}
			}
			else {
				for (size_t i = 0; i < count; i++) {
					const auto& value = gen();
					std_deque.emplace_front(value);
					arr_deque.emplace_front(value);
				}
			}

			return;
		}

		//a == action::remove
		if (e == end::front) {
			for (size_t i = 0; i < count; i++) {
				std_deque.pop_front();
				arr_deque.pop_front();
			}
		}
		else {
			for (size_t i = 0; i < count; i++) {
				std_deque.pop_back();
				arr_deque.pop_back();
			}
		}
	};

	for (auto step : steps) {
		cb(std::get<0>(step), std::get<1>(step), std::get<2>(step));
		assert_true(deque_equal(arr_deque, std_deque));
	}
}


void concurrencpp::tests::test_array_deque_combo() {
	test_array_deque_combo_impl<int>();
	test_array_deque_combo_impl<std::string>();
}

void concurrencpp::tests::test_array_deque() {
	tester tester("array_deque test");

	tester.add_step("default constructor", test_array_deque_default_constructor);
	tester.add_step("move constructor", test_array_deque_move_constructor);
	tester.add_step("destructor", test_array_deque_destructor);
	tester.add_step("clear", test_array_deque_clear);
	tester.add_step("emplace_back", test_array_deque_emplace_back);
	tester.add_step("pop_back", test_array_deque_pop_back);
	tester.add_step("emplace_front", test_array_deque_emplace_front);
	tester.add_step("pop_front", test_array_deque_pop_front);
	tester.add_step("operator == / operator !=", test_array_deque_equal_not_equal);
	tester.add_step("array_deque combo", test_array_deque_combo);

	tester.launch_test();
}