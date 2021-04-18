#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"

#include <unordered_set>

namespace concurrencpp::tests {
    void test_resume_on_shared_ptr();
    void test_resume_on_ref();
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    result<void> null_ptr_test_coroutine(std::shared_ptr<concurrencpp::executor> executor) {
        co_await concurrencpp::resume_on(executor);
    }

    result<void> ptr_test_coroutine(std::span<std::shared_ptr<concurrencpp::executor>> executors, std::unordered_set<size_t>& set)
    {
        for (auto& executor : executors)
        {
            co_await concurrencpp::resume_on(executor);
            set.insert(::concurrencpp::details::thread::get_current_virtual_id());
        }
    }

     result<void> ref_test_coroutine(std::span<concurrencpp::executor*> executors,
                                    std::unordered_set<size_t>& set) {
        for (auto executor : executors) {
            co_await concurrencpp::resume_on(*executor);
            set.insert(::concurrencpp::details::thread::get_current_virtual_id());
        }
    }
}  // namespace concurrencpp::tests

void concurrencpp::tests::test_resume_on_shared_ptr()
{
    assert_throws_with_error_message<std::invalid_argument>([] {
        null_ptr_test_coroutine({}).get();
    }, concurrencpp::details::consts::k_resume_on_null_exception_err_msg);

    concurrencpp::runtime runtime;
    std::shared_ptr<concurrencpp::executor> executors[4];

    executors[0] = runtime.thread_executor();
    executors[1] = runtime.thread_pool_executor();
    executors[2] = runtime.make_worker_thread_executor();
    executors[3] = runtime.make_worker_thread_executor();

	std::unordered_set<size_t> set;
    ptr_test_coroutine(executors, set).get();

	assert_equal(set.size(), std::size(executors));
}

void concurrencpp::tests::test_resume_on_ref() {
    concurrencpp::runtime runtime;
    concurrencpp::executor* executors[4];

    executors[0] = runtime.thread_executor().get();
    executors[1] = runtime.thread_pool_executor().get();
    executors[2] = runtime.make_worker_thread_executor().get();
    executors[3] = runtime.make_worker_thread_executor().get();

    std::unordered_set<size_t> set;
    ref_test_coroutine(executors, set).get();

    assert_equal(set.size(), std::size(executors));
}

using namespace concurrencpp::tests;

int main() {
    tester tester("resume_on");

    tester.add_step("resume_on(std::shared_ptr)", test_resume_on_shared_ptr);
    tester.add_step("resume_on(&)", test_resume_on_ref);

    tester.launch_test();
    return 0;
}