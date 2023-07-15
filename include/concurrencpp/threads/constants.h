#ifndef CONCURRENCPP_THREAD_CONSTS_H
#define CONCURRENCPP_THREAD_CONSTS_H

namespace concurrencpp::details::consts {
    inline const char* k_async_lock_null_resume_executor_err_msg = "concurrencpp::async_lock::lock() - given resume executor is null.";
    inline const char* k_async_lock_unlock_invalid_lock_err_msg = "concurrencpp::async_lock::unlock() - trying to unlock an unowned lock.";

    inline const char* k_scoped_async_lock_null_resume_executor_err_msg = "concurrencpp::scoped_async_lock::lock() - given resume executor is null.";

    inline const char* k_scoped_async_lock_lock_deadlock_err_msg = "concurrencpp::scoped_async_lock::lock() - *this is already locked.";

    inline const char* k_scoped_async_lock_lock_no_mutex_err_msg =
        "concurrencpp::scoped_async_lock::lock() - *this doesn't reference any async_lock.";

    inline const char* k_scoped_async_lock_try_lock_deadlock_err_msg = "concurrencpp::scoped_async_lock::try_lock() - *this is already locked.";

    inline const char* k_scoped_async_lock_try_lock_no_mutex_err_msg =
        "concurrencpp::scoped_async_lock::try_lock() - *this doesn't reference any async_lock.";

    inline const char* k_scoped_async_lock_unlock_invalid_lock_err_msg =
        "concurrencpp::scoped_async_lock::unlock() - trying to unlock an unowned lock.";

    inline const char* k_async_condition_variable_await_invalid_resume_executor_err_msg =
        "concurrencpp::async_condition_variable::await() - resume_executor is null.";

    inline const char* k_async_condition_variable_await_lock_unlocked_err_msg =
        "concurrencpp::async_condition_variable::await() - lock is unlocked.";

}  // namespace concurrencpp::details::consts

#endif