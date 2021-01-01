#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tests/test_utils/test_ready_result.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/random.h"
#include "helpers/object_observer.h"

namespace concurrencpp::tests {
    template<class type>
    void test_make_ready_result_impl();
    void test_make_ready_result();

    template<class type>
    void test_make_exceptional_result_impl();
    void test_make_exceptional_result();
}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_make_ready_result_impl() {
    result<type> result;

    if constexpr (std::is_same_v<void, type>) {
        result = concurrencpp::make_ready_result<type>();
    } else {
        result = make_ready_result<type>(result_factory<type>::get());
    }

    test_ready_result(std::move(result));
}

void concurrencpp::tests::test_make_ready_result() {
    test_make_ready_result_impl<int>();
    test_make_ready_result_impl<std::string>();
    test_make_ready_result_impl<void>();
    test_make_ready_result_impl<int&>();
    test_make_ready_result_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_make_exceptional_result_impl() {
    // empty exception_ptr makes make_exceptional_result throw.
    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            make_exceptional_result<type>({});
        },
        concurrencpp::details::consts::k_make_exceptional_result_exception_null_error_msg);

    const size_t id = 123456789;
    auto res0 = make_exceptional_result<type>(costume_exception(id));
    test_ready_result_costume_exception(std::move(res0), id);

    auto res1 = make_exceptional_result<type>(std::make_exception_ptr(costume_exception(id)));
    test_ready_result_costume_exception(std::move(res1), id);
}

void concurrencpp::tests::test_make_exceptional_result() {
    test_make_exceptional_result_impl<int>();
    test_make_exceptional_result_impl<std::string>();
    test_make_exceptional_result_impl<void>();
    test_make_exceptional_result_impl<int&>();
    test_make_exceptional_result_impl<std::string&>();
}

void concurrencpp::tests::test_make_result() {
    tester tester("make_result test");

    tester.add_step("make_ready_result", test_make_ready_result);
    tester.add_step("make_exceptional_result", test_make_exceptional_result);

    tester.launch_test();
}
