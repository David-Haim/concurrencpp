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

    inline const char* k_when_any_empty_range_error_msg = "concurrencpp::when_any() - given range contains no elements.";

}  // namespace concurrencpp::details::consts

#endif
