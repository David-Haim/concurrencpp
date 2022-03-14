#ifndef CONCURRENCPP_THREAD_CONSTS_H
#define CONCURRENCPP_THREAD_CONSTS_H

namespace concurrencpp::details::consts {
    inline const char* k_async_lock_null_resume_executor_err_msg = "async_lock::lock() - given resume executor is null.";
    inline const char* k_async_lock_unlock_invalid_lock_err_msg = "async_lock::unlock() - trying to unlock an unowned lock.";

    inline const char* k_scoped_async_lock_null_resume_executor_err_msg = "scoped_async_lock::lock() - given resume executor is null.";

    inline const char* k_scoped_async_lock_lock_deadlock_err_msg = "scoped_async_lock::lock() - *this is already locked.";

    inline const char* k_scoped_async_lock_lock_no_mutex_err_msg =
        "scoped_async_lock::lock() - *this doesn't reference any async_lock.";

    inline const char* k_scoped_async_lock_try_lock_deadlock_err_msg = "scoped_async_lock::try_lock() - *this is already locked.";

    inline const char* k_scoped_async_lock_try_lock_no_mutex_err_msg =
        "scoped_async_lock::try_lock() - *this doesn't reference any async_lock.";

    inline const char* k_scoped_async_lock_unlock_invalid_lock_err_msg =
        "scoped_async_lock::unlock() - trying to unlock an unowned lock.";

}  // namespace concurrencpp::details::consts

#endif