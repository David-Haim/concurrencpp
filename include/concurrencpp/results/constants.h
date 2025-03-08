#ifndef CONCURRENCPP_RESULT_CONSTS_H
#define CONCURRENCPP_RESULT_CONSTS_H

namespace concurrencpp::details::consts {
    /*
     * result_promise
     */

    inline const char* k_result_promise_get_result_already_retrieved_error_msg =
        "concurrencpp::result_promise::get_result() - result was already retrieved.";

    /*
     * result
     */
    
    inline const char* k_broken_task_exception_error_msg = "concurrencpp::result - associated task was interrupted abnormally";

    /*
     * when_xxx
     */

    inline const char* k_make_exceptional_result_exception_null_error_msg =
        "concurrencpp::make_exceptional_result() - given exception_ptr is null.";

    inline const char* k_make_exceptional_lazy_result_exception_null_error_msg =
        "concurrencpp::make_exceptional_lazy_result() - given exception_ptr is null.";

    inline const char* k_when_any_empty_range_error_msg = "concurrencpp::when_any() - given range contains no elements.";

    /*
     * resume_on
     */

    inline const char* k_resume_on_null_exception_err_msg = "concurrencpp::resume_on - given executor is null.";

    /*
     * parallel-coroutine
     */
    inline const char* k_parallel_coroutine_null_exception_err_msg = "concurrencpp::parallel-coroutine - given executor is null.";

}  // namespace concurrencpp::details::consts

#endif
