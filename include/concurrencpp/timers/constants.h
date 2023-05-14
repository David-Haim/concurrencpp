#ifndef CONCURRENCPP_TIMER_CONSTS_H
#define CONCURRENCPP_TIMER_CONSTS_H

namespace concurrencpp::details::consts {
    inline const char* k_timer_empty_get_due_time_err_msg = "concurrencpp::timer::get_due_time() - timer is empty.";
    inline const char* k_timer_empty_get_frequency_err_msg = "concurrencpp::timer::get_frequency() - timer is empty.";
    inline const char* k_timer_empty_get_executor_err_msg = "concurrencpp::timer::get_executor() - timer is empty.";
    inline const char* k_timer_empty_get_timer_queue_err_msg = "concurrencpp::timer::get_timer_queue() - timer is empty.";
    inline const char* k_timer_empty_set_frequency_err_msg = "concurrencpp::timer::set_frequency() - timer is empty.";

    inline const char* k_timer_queue_make_timer_executor_null_err_msg = "concurrencpp::timer_queue::make_timer() - executor is null.";
    inline const char* k_timer_queue_make_oneshot_timer_executor_null_err_msg =
        "concurrencpp::timer_queue::make_one_shot_timer() - executor is null.";
    inline const char* k_timer_queue_make_delay_object_executor_null_err_msg = "concurrencpp::timer_queue::make_delay_object() - executor is null.";
    inline const char* k_timer_queue_shutdown_err_msg = "concurrencpp::timer_queue has been shut down.";
}  // namespace concurrencpp::details::consts

#endif
