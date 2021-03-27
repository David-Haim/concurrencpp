#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/test_ready_result.h"

namespace concurrencpp::tests {
    template<class type>
    void test_shared_result_constructor_impl();
    void test_shared_result_constructor();

    template<class type>
    void test_shared_result_status_impl();
    void test_shared_result_status();

    template<class type>
    void test_shared_result_get_impl();
    void test_shared_result_get();

    template<class type>
    void test_shared_result_wait_impl();
    void test_shared_result_wait();

    template<class type>
    void test_shared_result_wait_for_impl();
    void test_shared_result_wait_for();

    template<class type>
    void test_shared_result_wait_until_impl();
    void test_shared_result_wait_until();

    template<class type>
    void test_shared_result_assignment_operator_empty_to_empty_move();
    template<class type>
    void test_shared_result_assignment_operator_non_empty_to_non_empty_move();
    template<class type>
    void test_shared_result_assignment_operator_empty_to_non_empty_move();
    template<class type>
    void test_shared_result_assignment_operator_non_empty_to_empty_move();
    template<class type>
    void test_shared_result_assignment_operator_assign_to_self_move();

    template<class type>
    void test_shared_result_assignment_operator_empty_to_empty_copy();
    template<class type>
    void test_shared_result_assignment_operator_non_empty_to_non_empty_copy();
    template<class type>
    void test_shared_result_assignment_operator_empty_to_non_empty_copy();
    template<class type>
    void test_shared_result_assignment_operator_non_empty_to_empty_copy();
    template<class type>
    void test_shared_result_assignment_operator_assign_to_self_copy();

    template<class type>
    void test_shared_result_assignment_operator_impl();
    void test_shared_result_assignment_operator();
}  // namespace concurrencpp::tests

using concurrencpp::result;
using concurrencpp::result_promise;
using namespace std::chrono;
using namespace concurrencpp::tests;

template<class type>
void concurrencpp::tests::test_shared_result_constructor_impl() {
    shared_result<type> default_constructed_result;
    assert_false(static_cast<bool>(default_constructed_result));

    // from result
    result_promise<type> rp;
    auto result = rp.get_result();
    shared_result<type> sr(std::move(result));

    assert_true(static_cast<bool>(sr));
    assert_equal(sr.status(), result_status::idle);

    // copy
    auto copy_result = sr;
    assert_true(static_cast<bool>(copy_result));
    assert_equal(sr.status(), result_status::idle);

    // move
    auto new_result = std::move(sr);
    assert_false(static_cast<bool>(sr));
    assert_true(static_cast<bool>(new_result));
    assert_equal(new_result.status(), result_status::idle);
}

void concurrencpp::tests::test_shared_result_constructor() {
    test_shared_result_constructor_impl<int>();
    test_shared_result_constructor_impl<std::string>();
    test_shared_result_constructor_impl<void>();
    test_shared_result_constructor_impl<int&>();
    test_shared_result_constructor_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_shared_result_status_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                shared_result<type>().status();
            },
            concurrencpp::details::consts::k_shared_result_status_error_msg);
    }

    // idle result
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        assert_equal(sr.status(), result_status::idle);
    }

    // ready by value
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        rp.set_from_function(value_gen<type>::default_value);
        assert_equal(sr.status(), result_status::value);
    }

    // exception result
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        rp.set_from_function(value_gen<type>::throw_ex);
        assert_equal(sr.status(), result_status::exception);
    }

    // multiple calls of status are ok
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        rp.set_from_function(value_gen<type>::default_value);

        for (size_t i = 0; i < 10; i++) {
            assert_equal(sr.status(), result_status::value);
        }
    }
}

void concurrencpp::tests::test_shared_result_status() {
    test_shared_result_status_impl<int>();
    test_shared_result_status_impl<std::string>();
    test_shared_result_status_impl<void>();
    test_shared_result_status_impl<int&>();
    test_shared_result_status_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_shared_result_get_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                shared_result<type>().get();
            },
            concurrencpp::details::consts::k_shared_result_get_error_msg);
    }

    // get blocks until value is present
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        sr.get();
        const auto now = high_resolution_clock::now();

        assert_false(static_cast<bool>(result));
        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));

        test_ready_result(std::move(sr));
        thread.join();
    }

    // get blocks until exception is present and empties the result
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        const auto id = 12345689;
        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), id, unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_exception(std::make_exception_ptr(custom_exception(id)));
        });

        try {
            sr.get();
        } catch (...) {
        }

        const auto now = high_resolution_clock::now();

        assert_false(static_cast<bool>(result));
        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));
        test_ready_result_custom_exception(std::move(sr), id);

        thread.join();
    }

    // if result is ready with value, get returns immediately
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        rp.set_from_function(value_gen<type>::default_value);

        const auto time_before = high_resolution_clock::now();

        sr.get();

        const auto time_after = high_resolution_clock::now();
        const auto total_blocking_time = duration_cast<milliseconds>(time_after - time_before).count();
        assert_false(static_cast<bool>(result));
        assert_smaller_equal(total_blocking_time, 10);
        test_ready_result(std::move(sr));
    }

    // if result is ready with exception, get returns immediately
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto id = 123456789;

        rp.set_exception(std::make_exception_ptr(custom_exception(id)));

        const auto time_before = high_resolution_clock::now();

        try {
            sr.get();
        } catch (...) {
        }

        const auto time_after = high_resolution_clock::now();
        const auto total_blocking_time = duration_cast<milliseconds>(time_after - time_before).count();

        assert_smaller_equal(total_blocking_time, 10);
        assert_false(static_cast<bool>(result));
        test_ready_result_custom_exception(std::move(sr), id);
    }

    // get can be called multiple times
    {
        shared_result<type> sr_val(result_gen<type>::ready());

        for (size_t i = 0; i < 6; i++) {
            sr_val.get();
            assert_true(static_cast<bool>(sr_val));
        }

        shared_result<type> sr_ex(make_exceptional_result<type>(std::make_exception_ptr(std::exception {})));

        for (size_t i = 0; i < 6; i++) {
            try {
                sr_ex.get();
            } catch (...) {
            }
            assert_true(static_cast<bool>(sr_ex));
        }
    }
}

void concurrencpp::tests::test_shared_result_get() {
    test_shared_result_get_impl<int>();
    test_shared_result_get_impl<std::string>();
    test_shared_result_get_impl<void>();
    test_shared_result_get_impl<int&>();
    test_shared_result_get_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_shared_result_wait_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                shared_result<type>().wait();
            },
            concurrencpp::details::consts::k_shared_result_wait_error_msg);
    }

    // wait blocks until value is present
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        sr.wait();
        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));

        test_ready_result(std::move(sr));
        thread.join();
    }

    // wait blocks until exception is present
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        const auto id = 123456789;
        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), id, unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_exception(std::make_exception_ptr(custom_exception(id)));
        });

        sr.wait();
        const auto now = high_resolution_clock::now();

        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));

        test_ready_result_custom_exception(std::move(sr), id);
        thread.join();
    }

    // if result is ready with value, wait returns immediately
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        rp.set_from_function(value_gen<type>::default_value);

        const auto time_before = high_resolution_clock::now();
        sr.wait();
        const auto time_after = high_resolution_clock::now();
        const auto total_blocking_time = duration_cast<milliseconds>(time_after - time_before).count();

        assert_smaller_equal(total_blocking_time, 5);
        test_ready_result(std::move(sr));
    }

    // if result is ready with exception, wait returns immediately
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto id = 123456789;

        rp.set_exception(std::make_exception_ptr(custom_exception(id)));

        const auto time_before = high_resolution_clock::now();
        sr.wait();
        const auto time_after = high_resolution_clock::now();
        const auto total_blocking_time = duration_cast<milliseconds>(time_after - time_before).count();

        assert_smaller_equal(total_blocking_time, 5);
        test_ready_result_custom_exception(std::move(sr), id);
    }

    // multiple calls to wait are ok
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(50);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        for (size_t i = 0; i < 10; i++) {
            sr.wait();
        }

        test_ready_result(std::move(sr));
        thread.join();
    }
}

void concurrencpp::tests::test_shared_result_wait() {
    test_shared_result_wait_impl<int>();
    test_shared_result_wait_impl<std::string>();
    test_shared_result_wait_impl<void>();
    test_shared_result_wait_impl<int&>();
    test_shared_result_wait_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_shared_result_wait_for_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                shared_result<type>().wait_for(seconds(1));
            },
            concurrencpp::details::consts::k_shared_result_wait_for_error_msg);
    }

    // if the result is ready by value, don't block and return status::value
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        rp.set_from_function(value_gen<type>::default_value);

        const auto before = high_resolution_clock::now();
        const auto status = sr.wait_for(seconds(10));
        const auto after = high_resolution_clock::now();
        const auto time = duration_cast<milliseconds>(after - before).count();

        assert_smaller_equal(time, 20);
        assert_equal(status, result_status::value);
        test_ready_result(std::move(sr));
    }

    // if the result is ready by exception, don't block and return status::exception
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const size_t id = 123456789;

        rp.set_exception(std::make_exception_ptr(custom_exception(id)));

        const auto before = high_resolution_clock::now();
        const auto status = sr.wait_for(seconds(10));
        const auto after = high_resolution_clock::now();
        const auto time = duration_cast<milliseconds>(after - before).count();

        assert_smaller_equal(time, 20);
        assert_equal(status, result_status::exception);
        test_ready_result_custom_exception(std::move(sr), id);
    }

    // if timeout reaches and no value/exception - return status::idle
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto waiting_time = milliseconds(50);
        const auto before = high_resolution_clock::now();
        const auto status = sr.wait_for(waiting_time);
        const auto after = high_resolution_clock::now();
        const auto time = duration_cast<milliseconds>(after - before);

        assert_equal(status, result_status::idle);
        assert_bigger_equal(time, waiting_time);
    }

    // if result is set before timeout, unblock, and return status::value
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        sr.wait_for(seconds(10));
        const auto now = high_resolution_clock::now();

        test_ready_result(std::move(sr));
        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));
        thread.join();
    }

    // if exception is set before timeout, unblock, and return status::exception
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        const auto id = 123456789;
        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), unblocking_time, id]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_exception(std::make_exception_ptr(custom_exception(id)));
        });

        sr.wait_for(seconds(10));
        const auto now = high_resolution_clock::now();

        test_ready_result_custom_exception(std::move(sr), id);
        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));
        thread.join();
    }

    // multiple calls of wait_for are ok
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        for (size_t i = 0; i < 10; i++) {
            sr.wait_for(milliseconds(10));
        }

        thread.join();
    }
}

void concurrencpp::tests::test_shared_result_wait_for() {
    test_shared_result_wait_for_impl<int>();
    test_shared_result_wait_for_impl<std::string>();
    test_shared_result_wait_for_impl<void>();
    test_shared_result_wait_for_impl<int&>();
    test_shared_result_wait_for_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_shared_result_wait_until_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                const auto later = high_resolution_clock::now() + seconds(10);
                shared_result<type>().wait_until(later);
            },
            concurrencpp::details::consts::k_shared_result_wait_until_error_msg);
    }

    // if time_point <= now, the function is equivalent to result::status
    {
        result_promise<type> rp_idle, rp_val, rp_err;
        result<type> idle_result = rp_idle.get_result(), value_result = rp_val.get_result(), err_result = rp_err.get_result();
        shared_result<type> shared_idle_result(std::move(idle_result)), shared_value_result(std::move(value_result)),
            shared_err_result(std::move(err_result));

        rp_val.set_from_function(value_gen<type>::default_value);
        rp_err.set_from_function(value_gen<type>::throw_ex);

        const auto now = high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        assert_equal(shared_idle_result.wait_until(now), concurrencpp::result_status::idle);
        assert_equal(shared_value_result.wait_until(now), concurrencpp::result_status::value);
        assert_equal(shared_err_result.wait_until(now), concurrencpp::result_status::exception);
    }

    // if the result is ready by value, don't block and return status::value
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        rp.set_from_function(value_gen<type>::default_value);

        const auto later = high_resolution_clock::now() + seconds(10);

        const auto before = high_resolution_clock::now();
        const auto status = sr.wait_until(later);
        const auto after = high_resolution_clock::now();

        const auto ms = duration_cast<milliseconds>(after - before).count();

        assert_smaller_equal(ms, 20);
        assert_equal(status, result_status::value);
        test_ready_result(std::move(sr));
    }

    // if the result is ready by exception, don't block and return status::exception
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        const size_t id = 123456789;

        rp.set_exception(std::make_exception_ptr(custom_exception(id)));

        const auto later = high_resolution_clock::now() + seconds(10);

        const auto before = high_resolution_clock::now();
        const auto status = sr.wait_until(later);
        const auto after = high_resolution_clock::now();

        const auto time = duration_cast<milliseconds>(after - before).count();

        assert_smaller_equal(time, 20);
        assert_equal(status, result_status::exception);
        test_ready_result_custom_exception(std::move(sr), id);
    }

    // if timeout reaches and no value/exception - return status::idle
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto later = high_resolution_clock::now() + milliseconds(150);
        const auto status = sr.wait_until(later);
        const auto now = high_resolution_clock::now();

        const auto ms = duration_cast<microseconds>(now - later).count();
        assert_equal(status, result_status::idle);
        assert_bigger_equal(now, later);
    }

    // if result is set before timeout, unblock, and return status::value
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);
        const auto later = high_resolution_clock::now() + seconds(10);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        sr.wait_until(later);
        const auto now = high_resolution_clock::now();

        test_ready_result(std::move(sr));
        assert_bigger_equal(now, unblocking_time);
        assert_smaller(now, unblocking_time + seconds(1));
        thread.join();
    }

    // if exception is set before timeout, unblock, and return status::exception
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));
        const auto id = 123456789;

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);
        const auto later = high_resolution_clock::now() + seconds(10);

        std::thread thread([rp = std::move(rp), unblocking_time, id]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_exception(std::make_exception_ptr(custom_exception(id)));
        });

        sr.wait_until(later);
        const auto now = high_resolution_clock::now();

        test_ready_result_custom_exception(std::move(sr), id);
        assert_bigger_equal(now, unblocking_time);
        assert_smaller_equal(now, unblocking_time + seconds(1));
        thread.join();
    }

    // multiple calls to wait_until are ok
    {
        result_promise<type> rp;
        auto result = rp.get_result();
        shared_result<type> sr(std::move(result));

        const auto unblocking_time = high_resolution_clock::now() + milliseconds(150);

        std::thread thread([rp = std::move(rp), unblocking_time]() mutable {
            std::this_thread::sleep_until(unblocking_time);
            rp.set_from_function(value_gen<type>::default_value);
        });

        for (size_t i = 0; i < 10; i++) {
            const auto later = high_resolution_clock::now() + milliseconds(50);
            sr.wait_until(later);
        }

        thread.join();
    }
}

void concurrencpp::tests::test_shared_result_wait_until() {
    test_shared_result_wait_until_impl<int>();
    test_shared_result_wait_until_impl<std::string>();
    test_shared_result_wait_until_impl<void>();
    test_shared_result_wait_until_impl<int&>();
    test_shared_result_wait_until_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_empty_to_empty_move() {
    shared_result<type> result_0, result_1;
    result_0 = std::move(result_1);
    assert_false(static_cast<bool>(result_0));
    assert_false(static_cast<bool>(result_1));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_non_empty_to_non_empty_move() {
    result_promise<type> rp_0, rp_1;
    result<type> result_0 = rp_0.get_result(), result_1 = rp_1.get_result();
    shared_result<type> sr0(std::move(result_0)), sr1(std::move(result_1));

    sr0 = std::move(sr1);

    assert_false(static_cast<bool>(sr1));
    assert_true(static_cast<bool>(sr0));

    rp_0.set_from_function(value_gen<type>::default_value);
    assert_equal(sr0.status(), result_status::idle);

    rp_1.set_from_function(value_gen<type>::default_value);
    test_ready_result(std::move(sr0));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_empty_to_non_empty_move() {
    result_promise<type> rp_0;
    result<type> result_0 = rp_0.get_result(), result_1;
    shared_result<type> sr0(std::move(result_0)), sr1(std::move(result_1));

    sr0 = std::move(sr1);
    assert_false(static_cast<bool>(sr0));
    assert_false(static_cast<bool>(sr1));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_non_empty_to_empty_move() {
    result_promise<type> rp_1;
    result<type> result_0, result_1 = rp_1.get_result();
    shared_result<type> sr0(std::move(result_0)), sr1(std::move(result_1));

    sr0 = std::move(sr1);
    assert_true(static_cast<bool>(sr0));
    assert_false(static_cast<bool>(sr1));

    rp_1.set_from_function(value_gen<type>::default_value);
    test_ready_result(std::move(sr0));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_assign_to_self_move() {
    shared_result<type> empty;

    empty = std::move(empty);
    assert_false(static_cast<bool>(empty));

    result_promise<type> rp_1;
    auto res1 = rp_1.get_result();
    shared_result<type> non_empty(std::move(res1));

    non_empty = std::move(non_empty);
    assert_true(static_cast<bool>(non_empty));

    auto copy = non_empty;
    copy = std::move(non_empty);
    assert_true(static_cast<bool>(copy));
    assert_true(static_cast<bool>(non_empty));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_empty_to_empty_copy() {
    shared_result<type> result_0, result_1;
    result_0 = result_1;
    assert_false(static_cast<bool>(result_0));
    assert_false(static_cast<bool>(result_1));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_non_empty_to_non_empty_copy() {
    result_promise<type> rp_0, rp_1;
    result<type> result_0 = rp_0.get_result(), result_1 = rp_1.get_result();
    shared_result<type> sr0(std::move(result_0)), sr1(std::move(result_1));

    sr0 = sr1;

    assert_true(static_cast<bool>(sr1));
    assert_true(static_cast<bool>(sr0));

    rp_0.set_from_function(value_gen<type>::default_value);
    assert_equal(sr0.status(), result_status::idle);

    rp_1.set_from_function(value_gen<type>::default_value);
    test_ready_result(std::move(sr0));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_empty_to_non_empty_copy() {
    result_promise<type> rp_0;
    result<type> result_0 = rp_0.get_result(), result_1;
    shared_result<type> sr0(std::move(result_0)), sr1(std::move(result_1));

    sr0 = sr1;
    assert_false(static_cast<bool>(sr0));
    assert_false(static_cast<bool>(sr1));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_non_empty_to_empty_copy() {
    result_promise<type> rp_1;
    result<type> result_0, result_1 = rp_1.get_result();
    shared_result<type> sr0(std::move(result_0)), sr1(std::move(result_1));

    sr0 = sr1;
    assert_true(static_cast<bool>(sr0));
    assert_true(static_cast<bool>(sr1));

    rp_1.set_from_function(value_gen<type>::default_value);
    test_ready_result(std::move(sr0));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_assign_to_self_copy() {
    shared_result<type> empty;

    empty = empty;
    assert_false(static_cast<bool>(empty));

    result_promise<type> rp_1;
    auto res1 = rp_1.get_result();
    shared_result<type> non_empty(std::move(res1));

    non_empty = non_empty;
    assert_true(static_cast<bool>(non_empty));

    auto copy = non_empty;
    copy = non_empty;
    assert_true(static_cast<bool>(copy));
    assert_true(static_cast<bool>(non_empty));
}

template<class type>
void concurrencpp::tests::test_shared_result_assignment_operator_impl() {
    test_shared_result_assignment_operator_empty_to_empty_move<type>();
    test_shared_result_assignment_operator_non_empty_to_empty_move<type>();
    test_shared_result_assignment_operator_empty_to_non_empty_move<type>();
    test_shared_result_assignment_operator_non_empty_to_non_empty_move<type>();
    test_shared_result_assignment_operator_assign_to_self_move<type>();

    test_shared_result_assignment_operator_empty_to_empty_copy<type>();
    test_shared_result_assignment_operator_non_empty_to_empty_copy<type>();
    test_shared_result_assignment_operator_empty_to_non_empty_copy<type>();
    test_shared_result_assignment_operator_non_empty_to_non_empty_copy<type>();
    test_shared_result_assignment_operator_assign_to_self_copy<type>();
}

void concurrencpp::tests::test_shared_result_assignment_operator() {
    test_shared_result_assignment_operator_impl<int>();
    test_shared_result_assignment_operator_impl<std::string>();
    test_shared_result_assignment_operator_impl<void>();
    test_shared_result_assignment_operator_impl<int&>();
    test_shared_result_assignment_operator_impl<std::string&>();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("shared_result test");

    tester.add_step("constructor", test_shared_result_constructor);
    tester.add_step("status", test_shared_result_status);
    tester.add_step("get", test_shared_result_get);
    tester.add_step("wait", test_shared_result_wait);
    tester.add_step("wait_for", test_shared_result_wait_for);
    tester.add_step("wait_until", test_shared_result_wait_until);
    tester.add_step("operator =", test_shared_result_assignment_operator);

    tester.launch_test();
    return 0;
}