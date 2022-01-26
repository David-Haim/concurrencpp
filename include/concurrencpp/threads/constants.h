#ifndef CONCURRENCPP_THREAD_CONSTS_H
#    define CONCURRENCPP_THREAD_CONSTS_H

namespace concurrencpp::details::consts {
    inline const char* k_async_lock_null_resume_executor_err_msg = "async_lock::lock() - given resume executor is null.";
    inline const char* k_async_lock_unlock_invalid_lock_err_msg = "async_lock::unlock() - trying to unlock an unowned lock.";
}  // namespace concurrencpp::details::consts

#endif