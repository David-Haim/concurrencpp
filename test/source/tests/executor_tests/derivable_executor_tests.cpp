#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/object_observer.h"
#include "tests/test_utils/result_factory.h"

namespace concurrencpp::tests {
    template<class executor_type>
    void test_derivable_executor_post_impl(std::shared_ptr<executor_type> executor, std::counting_semaphore<>& semaphore);
    void test_derivable_executor_post();

    template<class executor_type>
    void test_derivable_executor_submit_impl(std::shared_ptr<executor_type> executor);
    void test_derivable_executor_submit();

    template<class executor_type>
    void test_derivable_executor_bulk_post_impl(std::shared_ptr<executor_type> executor, std::counting_semaphore<>& semaphore);
    void test_derivable_executor_bulk_post();

    template<class executor_type>
    void test_derivable_executor_bulk_submit_impl(std::shared_ptr<executor_type> executor);
    void test_derivable_executor_bulk_submit();
}  // namespace concurrencpp::tests

template<class executor_type>
void concurrencpp::tests::test_derivable_executor_post_impl(std::shared_ptr<executor_type> executor, std::counting_semaphore<>& semaphore) {
    executor->post([&semaphore] {
        semaphore.release();
        return result_factory<int>::get();
    });

    executor->post([&semaphore] {
        semaphore.release();
        return result_factory<int>::throw_ex();
    });

    executor->post([&semaphore] {
        semaphore.release();
        return result_factory<std::string>::get();
    });

    executor->post([&semaphore] {
        semaphore.release();
        return result_factory<std::string>::throw_ex();
    });

    executor->post([&semaphore] {
        semaphore.release();
        return result_factory<void>::get();
    });

    executor->post([&semaphore] {
        semaphore.release();
        return result_factory<void>::throw_ex();
    });

    executor->post([&semaphore]() -> int& {
        semaphore.release();
        return result_factory<int&>::get();
    });

    executor->post([&semaphore]() -> int& {
        semaphore.release();
        return result_factory<int&>::throw_ex();
    });

    executor->post([&semaphore]() -> std::string& {
        semaphore.release();
        return result_factory<std::string&>::get();
    });

    executor->post([&semaphore]() -> std::string& {
        semaphore.release();
        return result_factory<std::string&>::throw_ex();
    });
}

void concurrencpp::tests::test_derivable_executor_post() {
    concurrencpp::runtime runtime;
    std::counting_semaphore<> semaphore(0);
    auto worker_thread_executor = runtime.make_worker_thread_executor();

    test_derivable_executor_post_impl(runtime.inline_executor(), semaphore);
    test_derivable_executor_post_impl(runtime.thread_executor(), semaphore);
    test_derivable_executor_post_impl(runtime.thread_pool_executor(), semaphore);
    test_derivable_executor_post_impl(worker_thread_executor, semaphore);

    for (size_t i = 0; i < 40; i++) {
        assert_true(semaphore.try_acquire_for(std::chrono::seconds(10)));
    }
}

template<class executor_type>
void concurrencpp::tests::test_derivable_executor_submit_impl(std::shared_ptr<executor_type> executor) {
    auto int_res_val = executor->submit([] {
        return result_factory<int>::get();
    });

    static_assert(std::is_same_v<decltype(int_res_val), result<int>>);

    auto int_res_ex = executor->submit([] {
        return result_factory<int>::throw_ex();
    });

    static_assert(std::is_same_v<decltype(int_res_ex), result<int>>);

    auto str_res_val = executor->submit([] {
        return result_factory<std::string>::get();
    });

    static_assert(std::is_same_v<decltype(str_res_val), result<std::string>>);

    auto str_res_ex = executor->submit([] {
        return result_factory<std::string>::throw_ex();
    });

    static_assert(std::is_same_v<decltype(str_res_ex), result<std::string>>);

    auto void_res_val = executor->submit([] {
        return result_factory<void>::get();
    });

    static_assert(std::is_same_v<decltype(void_res_val), result<void>>);

    auto void_res_ex = executor->submit([] {
        return result_factory<void>::throw_ex();
    });

    static_assert(std::is_same_v<decltype(void_res_ex), result<void>>);

    auto int_ref_res_val = executor->submit([]() -> int& {
        return result_factory<int&>::get();
    });

    static_assert(std::is_same_v<decltype(int_ref_res_val), result<int&>>);

    auto int_ref_res_ex = executor->submit([]() -> int& {
        return result_factory<int&>::throw_ex();
    });

    static_assert(std::is_same_v<decltype(int_ref_res_ex), result<int&>>);

    auto str_ref_res_val = executor->submit([]() -> std::string& {
        return result_factory<std::string&>::get();
    });

    static_assert(std::is_same_v<decltype(str_ref_res_val), result<std::string&>>);

    auto str_ref_res_ex = executor->submit([]() -> std::string& {
        return result_factory<std::string&>::throw_ex();
    });

    static_assert(std::is_same_v<decltype(str_ref_res_ex), result<std::string&>>);

    assert_equal(int_res_val.get(), result_factory<int>::get());
    assert_equal(str_res_val.get(), result_factory<std::string>::get());

    void_res_val.get();

    assert_equal(&int_ref_res_val.get(), &result_factory<int&>::get());
    assert_equal(&str_ref_res_val.get(), &result_factory<std::string&>::get());

    assert_throws<test_exception>([&] {
        int_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        str_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        void_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        int_ref_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        str_ref_res_ex.get();
    });
}

void concurrencpp::tests::test_derivable_executor_submit() {
    concurrencpp::runtime runtime;
    auto worker_thread_executor = runtime.make_worker_thread_executor();
}

namespace concurrencpp::tests {
    template<class type>
    struct result_producer {
        std::counting_semaphore<>* semaphore;
        const bool terminate_by_exception;

        result_producer(bool terminate_by_exception) noexcept : semaphore(nullptr), terminate_by_exception(terminate_by_exception) {}

        result_producer(bool terminate_by_exception, std::counting_semaphore<>* semaphore) noexcept :
            semaphore(semaphore), terminate_by_exception(terminate_by_exception) {}

        type operator()() {
            if (semaphore != nullptr) {
                semaphore->release();
            }

            if (terminate_by_exception) {
                return result_factory<type>::throw_ex();
            }

            return result_factory<type>::get();
        }
    };
}  // namespace concurrencpp::tests

template<class executor_type>
void concurrencpp::tests::test_derivable_executor_bulk_post_impl(std::shared_ptr<executor_type> executor, std::counting_semaphore<>& semaphore) {
    result_producer<int> int_fun[2] = {{false, &semaphore}, {true, &semaphore}};
    executor->bulk_post(std::span<result_producer<int>> {int_fun});

    result_producer<std::string> str_fun[2] = {{false, &semaphore}, {true, &semaphore}};
    executor->bulk_post(std::span<result_producer<std::string>> {str_fun});

    result_producer<void> void_fun[2] = {{false, &semaphore}, {true, &semaphore}};
    executor->bulk_post(std::span<result_producer<void>> {void_fun});

    result_producer<int&> int_ref_fun[2] = {{false, &semaphore}, {true, &semaphore}};
    executor->bulk_post(std::span<result_producer<int&>> {int_ref_fun});

    result_producer<std::string&> str_ref_fun[2] = {{false, &semaphore}, {true, &semaphore}};
    executor->bulk_post(std::span<result_producer<std::string&>> {str_ref_fun});
}

void concurrencpp::tests::test_derivable_executor_bulk_post() {
    concurrencpp::runtime runtime;
    std::counting_semaphore<> semaphore(0);
    auto worker_thread_executor = runtime.make_worker_thread_executor();

    test_derivable_executor_bulk_post_impl(runtime.inline_executor(), semaphore);
    test_derivable_executor_bulk_post_impl(runtime.thread_executor(), semaphore);
    test_derivable_executor_bulk_post_impl(runtime.thread_pool_executor(), semaphore);
    test_derivable_executor_bulk_post_impl(worker_thread_executor, semaphore);

    for (size_t i = 0; i < 40; i++) {
        assert_true(semaphore.try_acquire_for(std::chrono::seconds(10)));
    }
}

template<class executor_type>
void concurrencpp::tests::test_derivable_executor_bulk_submit_impl(std::shared_ptr<executor_type> executor) {
    result_producer<int> int_fun[2] = {{false}, {true}};
    auto int_res_vec = executor->bulk_submit(std::span<result_producer<int>> {int_fun});

    static_assert(std::is_same_v<decltype(int_res_vec), std::vector<result<int>>>);

    auto& int_res_val = int_res_vec[0];
    auto& int_res_ex = int_res_vec[1];

    result_producer<std::string> str_fun[2] = {{false}, {true}};
    auto str_res_vec = executor->bulk_submit(std::span<result_producer<std::string>> {str_fun});

    static_assert(std::is_same_v<decltype(str_res_vec), std::vector<result<std::string>>>);

    auto& str_res_val = str_res_vec[0];
    auto& str_res_ex = str_res_vec[1];

    result_producer<void> void_fun[2] = {{false}, {true}};
    auto void_res_vec = executor->bulk_submit(std::span<result_producer<void>> {void_fun});

    static_assert(std::is_same_v<decltype(void_res_vec), std::vector<result<void>>>);

    auto& void_res_val = void_res_vec[0];
    auto& void_res_ex = void_res_vec[1];

    result_producer<int&> int_ref_fun[2] = {{false}, {true}};
    auto int_ref_res_vec = executor->bulk_submit(std::span<result_producer<int&>> {int_ref_fun});

    static_assert(std::is_same_v<decltype(int_ref_res_vec), std::vector<result<int&>>>);

    auto& int_ref_res_val = int_ref_res_vec[0];
    auto& int_ref_res_ex = int_ref_res_vec[1];

    result_producer<std::string&> str_ref_fun[2] = {{false}, {true}};
    auto str_ref_res_vec = executor->bulk_submit(std::span<result_producer<std::string&>> {str_ref_fun});

    static_assert(std::is_same_v<decltype(str_ref_res_vec), std::vector<result<std::string&>>>);

    auto& str_ref_res_val = str_ref_res_vec[0];
    auto& str_ref_res_ex = str_ref_res_vec[1];

    assert_equal(int_res_val.get(), result_factory<int>::get());
    assert_equal(str_res_val.get(), result_factory<std::string>::get());

    void_res_val.get();

    assert_equal(&int_ref_res_val.get(), &result_factory<int&>::get());
    assert_equal(&str_ref_res_val.get(), &result_factory<std::string&>::get());

    assert_throws<test_exception>([&] {
        int_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        str_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        void_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        int_ref_res_ex.get();
    });

    assert_throws<test_exception>([&] {
        str_ref_res_ex.get();
    });
}

void concurrencpp::tests::test_derivable_executor_bulk_submit() {
    concurrencpp::runtime runtime;
    auto worker_thread_executor = runtime.make_worker_thread_executor();

    test_derivable_executor_bulk_submit_impl(runtime.inline_executor());
    test_derivable_executor_bulk_submit_impl(runtime.thread_executor());
    test_derivable_executor_bulk_submit_impl(runtime.thread_pool_executor());
    test_derivable_executor_bulk_submit_impl(worker_thread_executor);
}

void concurrencpp::tests::test_derivable_executor() {
    tester tester("derivable_executor test");

    tester.add_step("post", test_derivable_executor_post);
    tester.add_step("submit", test_derivable_executor_submit);
    tester.add_step("bulk_post", test_derivable_executor_bulk_post);
    tester.add_step("bulk_submit", test_derivable_executor_bulk_submit);

    tester.launch_test();
}
