#ifndef CONCURRENCPP_RESULT_CONSTS_H
#define CONCURRENCPP_RESULT_CONSTS_H

namespace concurrencpp::details::consts {
    /*
     * result_promise
     */

    inline const char* k_result_promise_set_result_error_msg = "concurrencpp::result_promise::set_result() - empty result_promise.";

    inline const char* k_result_promise_set_exception_error_msg = "concurrencpp::result_promise::set_exception() - empty result_promise.";

    inline const char* k_result_promise_set_exception_null_exception_error_msg =
        "concurrencpp::result_promise::set_exception() - exception pointer is null.";

    inline const char* k_result_promise_set_from_function_error_msg = "concurrencpp::result_promise::set_from_function() - empty result_promise.";

    inline const char* k_result_promise_get_result_error_msg = "concurrencpp::result_promise::get_result() - empty result_promise.";

    inline const char* k_result_promise_get_result_already_retrieved_error_msg =
        "concurrencpp::result_promise::get_result() - result was already retrieved.";

    /*
     * result
     */

    inline const char* k_result_status_error_msg = "concurrencpp::result::status() - result is empty.";

    inline const char* k_result_get_error_msg = "concurrencpp::result::get() - result is empty.";

    inline const char* k_result_wait_error_msg = "concurrencpp::result::wait() - result is empty.";

    inline const char* k_result_wait_for_error_msg = "concurrencpp::result::wait_for() - result is empty.";

    inline const char* k_result_wait_until_error_msg = "concurrencpp::result::wait_until() - result is empty.";

    inline const char* k_result_operator_co_await_error_msg = "concurrencpp::result::operator co_await() - result is empty.";

    inline const char* k_result_resolve_error_msg = "concurrencpp::result::resolve() - result is empty.";

    inline const char* k_executor_exception_error_msg =
        "concurrencpp::concurrencpp::result - an exception was thrown while trying to enqueue result continuation.";

    inline const char* k_broken_task_exception_error_msg = "concurrencpp::result - associated task was interrupted abnormally";

    /*
     * when_xxx
     */

    inline const char* k_make_exceptional_result_exception_null_error_msg = "concurrencpp::make_exception_result() - given exception_ptr is null.";

    inline const char* k_when_all_empty_result_error_msg = "concurrencpp::when_all() - one of the result objects is empty.";

    inline const char* k_when_all_null_resume_executor_error_msg = "concurrencpp::when_all() - given resume_executor is null.";

    inline const char* k_when_any_empty_result_error_msg = "concurrencpp::when_any() - one of the result objects is empty.";

    inline const char* k_when_any_empty_range_error_msg = "concurrencpp::when_any() - given range contains no elements.";

    inline const char* k_when_any_null_resume_executor_error_msg = "concurrencpp::when_any() - given resume_executor is null.";

    /*
     * shared_result
     */

    inline const char* k_shared_result_status_error_msg = "concurrencpp::shared_result::status() - result is empty.";

    inline const char* k_shared_result_get_error_msg = "concurrencpp::shared_result::get() - result is empty.";

    inline const char* k_shared_result_wait_error_msg = "concurrencpp::shared_result::wait() - result is empty.";

    inline const char* k_shared_result_wait_for_error_msg = "concurrencpp::shared_result::wait_for() - result is empty.";

    inline const char* k_shared_result_wait_until_error_msg = "concurrencpp::shared_result::wait_until() - result is empty.";

    inline const char* k_shared_result_operator_co_await_error_msg = "concurrencpp::shared_result::operator co_await() - result is empty.";

    inline const char* k_shared_result_resolve_error_msg = "concurrencpp::shared_result::resolve() - result is empty.";

    /*
     * lazy_result
     */

    inline const char* k_empty_lazy_result_status_err_msg = "concurrencpp::lazy_result::status - result is empty.";

    inline const char* k_empty_lazy_result_operator_co_await_err_msg = "concurrencpp::lazy_result::operator co_await - result is empty.";

    inline const char* k_empty_lazy_result_resolve_err_msg = "concurrencpp::lazy_result::resolve - result is empty.";

    inline const char* k_empty_lazy_result_run_err_msg = "concurrencpp::lazy_result::run - result is empty.";

    /*
     * resume_on
     */

    inline const char* k_resume_on_null_exception_err_msg = "concurrencpp::resume_on - given executor is null.";

    /*
     * generator
     */
    inline const char* k_empty_generator_begin_err_msg = "concurrencpp::generator::begin - generator is empty.";

    /*
     * parallel-coroutine
     */
    inline const char* k_parallel_coroutine_null_exception_err_msg = "concurrencpp::parallel-coroutine - given executor is null.";

}  // namespace concurrencpp::details::consts

#endif
