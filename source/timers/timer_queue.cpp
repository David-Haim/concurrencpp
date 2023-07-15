#include "concurrencpp/timers/timer.h"
#include "concurrencpp/timers/timer_queue.h"

#include "concurrencpp/coroutines/coroutine.h"
#include "concurrencpp/executors/constants.h"
#include "concurrencpp/executors/executor.h"

#include <set>
#include <unordered_map>

#include <cassert>

using namespace std::chrono;

using concurrencpp::timer;
using concurrencpp::timer_queue;
using concurrencpp::details::timer_request;
using concurrencpp::details::timer_state_base;

using timer_ptr = timer_queue::timer_ptr;
using time_point = timer_queue::time_point;
using request_queue = timer_queue::request_queue;

namespace concurrencpp::details {
    namespace {
        struct deadline_comparator {
            bool operator()(const timer_ptr& a, const timer_ptr& b) const noexcept {
                return a->get_deadline() < b->get_deadline();
            }
        };

        class timer_queue_internal {
            using timer_set = std::multiset<timer_ptr, deadline_comparator>;
            using timer_set_iterator = typename timer_set::iterator;
            using iterator_map = std::unordered_map<timer_ptr, timer_set_iterator>;

           private:
            timer_set m_timers;
            iterator_map m_iterator_mapper;

            void add_timer_internal(timer_ptr new_timer) {
                assert(m_iterator_mapper.find(new_timer) == m_iterator_mapper.end());
                auto timer_it = m_timers.emplace(new_timer);
                m_iterator_mapper.emplace(std::move(new_timer), timer_it);
            }

            void remove_timer_internal(timer_ptr existing_timer) {
                auto timer_it = m_iterator_mapper.find(existing_timer);
                if (timer_it == m_iterator_mapper.end()) {
                    assert(existing_timer->is_oneshot() || existing_timer->cancelled());  // the timer was already deleted by
                                                                                          // the queue when it was fired.
                    return;
                }

                auto set_iterator = timer_it->second;
                m_timers.erase(set_iterator);
                m_iterator_mapper.erase(timer_it);
            }

            void process_request_queue(request_queue& queue) {
                for (auto& request : queue) {
                    auto& timer_ptr = request.first;
                    const auto opt = request.second;

                    if (opt == timer_request::add) {
                        add_timer_internal(std::move(timer_ptr));
                    } else {
                        remove_timer_internal(std::move(timer_ptr));
                    }
                }
            }

            void reset_containers_memory() noexcept {
                assert(empty());
                timer_set timers;
                std::swap(m_timers, timers);
                iterator_map iterator_mapper;
                std::swap(m_iterator_mapper, iterator_mapper);
            }

           public:
            bool empty() const noexcept {
                assert(m_iterator_mapper.size() == m_timers.size());
                return m_timers.empty();
            }

            ::time_point process_timers(request_queue& queue) {
                process_request_queue(queue);

                const auto now = high_resolution_clock::now();

                while (true) {
                    if (m_timers.empty()) {
                        break;
                    }

                    timer_set temp_set;

                    auto first_timer_it = m_timers.begin();  // closest deadline
                    auto timer_ptr = *first_timer_it;
                    const auto is_oneshot = timer_ptr->is_oneshot();

                    if (!timer_ptr->expired(now)) {
                        // if this timer is not expired, the next ones are guaranteed not to, as
                        // the set is ordered by deadlines.
                        break;
                    }

                    // we are going to modify the timer, so first we extract it
                    auto timer_node = m_timers.extract(first_timer_it);

                    // we cannot use the naked node_handle according to the standard. it must
                    // be contained somewhere.
                    auto temp_it = temp_set.insert(std::move(timer_node));

                    // we fire it only if it's not cancelled
                    const auto cancelled = timer_ptr->cancelled();
                    if (!cancelled) {
                        (*temp_it)->fire();
                    }

                    if (is_oneshot || cancelled) {
                        m_iterator_mapper.erase(timer_ptr);
                        continue;  // let the timer die inside temp_set
                    }

                    // regular timer, re-insert into the right position
                    timer_node = temp_set.extract(temp_it);
                    auto new_it = m_timers.insert(std::move(timer_node));
                    // AppleClang doesn't have std::unordered_map::contains yet
                    assert(m_iterator_mapper.find(timer_ptr) != m_iterator_mapper.end());
                    m_iterator_mapper[timer_ptr] = new_it;  // update the iterator map, multiset::extract invalidates the
                    // timer
                }

                if (m_timers.empty()) {
                    reset_containers_memory();
                    return now + std::chrono::hours(24);
                }

                // get the closest deadline.
                return (**m_timers.begin()).get_deadline();
            }
        };
    }  // namespace
}  // namespace concurrencpp::details

timer_queue::timer_queue(milliseconds max_waiting_time,
                         const std::function<void(std::string_view thread_name)>& thread_started_callback,
                         const std::function<void(std::string_view thread_name)>& thread_terminated_callback) :
    m_thread_started_callback(thread_started_callback),
    m_thread_terminated_callback(thread_terminated_callback), m_atomic_abort(false), m_abort(false), m_idle(true),
    m_max_waiting_time(max_waiting_time) {}

timer_queue::~timer_queue() noexcept {
    shutdown();
    assert(!m_worker.joinable());
}

void timer_queue::add_internal_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer) {
    assert(lock.owns_lock());
    m_request_queue.emplace_back(std::move(new_timer), timer_request::add);
    lock.unlock();

    m_condition.notify_one();
}

void timer_queue::remove_internal_timer(timer_ptr existing_timer) {
    {
        std::unique_lock<decltype(m_lock)> lock(m_lock);
        m_request_queue.emplace_back(std::move(existing_timer), timer_request::remove);
    }

    m_condition.notify_one();
}

void timer_queue::add_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer) {
    assert(lock.owns_lock());

    if (m_abort) {
        throw errors::runtime_shutdown(details::consts::k_timer_queue_shutdown_err_msg);
    }

    auto old_thread = ensure_worker_thread(lock);
    add_internal_timer(lock, new_timer);

    if (old_thread.joinable()) {
        old_thread.join();
    }
}

void timer_queue::work_loop() {
    time_point next_deadline;
    details::timer_queue_internal internal_state;

    while (true) {
        std::unique_lock<decltype(m_lock)> lock(m_lock);
        if (internal_state.empty()) {
            const auto res = m_condition.wait_for(lock, m_max_waiting_time, [this] {
                return !m_request_queue.empty() || m_abort;
            });

            if (!res) {
                m_idle = true;
                lock.unlock();
                return;
            }

        } else {
            m_condition.wait_until(lock, next_deadline, [this] {
                return !m_request_queue.empty() || m_abort;
            });
        }

        if (m_abort) {
            return;
        }

        auto request_queue = std::move(m_request_queue);
        lock.unlock();

        next_deadline = internal_state.process_timers(request_queue);
        const auto now = clock_type::now();
        if (next_deadline <= now) {
            continue;
        }
    }
}

bool timer_queue::shutdown_requested() const noexcept {
    return m_atomic_abort.load(std::memory_order_relaxed);
}

void timer_queue::shutdown() {
    const auto state_before = m_atomic_abort.exchange(true, std::memory_order_relaxed);
    if (state_before) {
        return;  // timer_queue has been shut down already.
    }

    std::unique_lock<std::mutex> lock(m_lock);
    m_abort = true;

    if (!m_worker.joinable()) {
        return;  // nothing to shut down
    }

    m_request_queue.clear();
    lock.unlock();

    m_condition.notify_all();
    m_worker.join();
}

concurrencpp::details::thread timer_queue::ensure_worker_thread(std::unique_lock<std::mutex>& lock) {
    assert(lock.owns_lock());
    if (!m_idle) {
        return {};
    }

    auto old_worker = std::move(m_worker);

    m_worker = details::thread(
        details::make_executor_worker_name(details::consts::k_timer_queue_name),
        [this] {
            work_loop();
        },
        m_thread_started_callback,
        m_thread_terminated_callback);

    m_idle = false;
    return old_worker;
}

concurrencpp::lazy_result<void> timer_queue::make_delay_object_impl(std::chrono::milliseconds due_time,
                                                                    std::shared_ptr<concurrencpp::timer_queue> self,
                                                                    std::shared_ptr<concurrencpp::executor> executor) {
    class delay_object_awaitable : public details::suspend_always {

       private:
        const size_t m_due_time_ms;
        timer_queue& m_parent_queue;
        std::shared_ptr<concurrencpp::executor> m_executor;
        bool m_interrupted = false;

       public:
        delay_object_awaitable(size_t due_time_ms,
                               timer_queue& parent_queue,
                               std::shared_ptr<concurrencpp::executor> executor) noexcept :
            m_due_time_ms(due_time_ms),
            m_parent_queue(parent_queue), m_executor(std::move(executor)) {}

        void await_suspend(details::coroutine_handle<void> coro_handle) noexcept {
            try {
                m_parent_queue.make_timer_impl(m_due_time_ms,
                                               0,
                                               std::move(m_executor),
                                               true,
                                               details::await_via_functor {coro_handle, &m_interrupted});

            } catch (...) {
                // do nothing. ~await_via_functor will resume the coroutine and throw an exception.
            }
        }

        void await_resume() const {
            if (m_interrupted) {
                throw errors::broken_task(details::consts::k_broken_task_exception_error_msg);
            }
        }
    };

    co_await delay_object_awaitable {static_cast<size_t>(due_time.count()), *this, std::move(executor)};
}

concurrencpp::lazy_result<void> timer_queue::make_delay_object(std::chrono::milliseconds due_time,
                                                               std::shared_ptr<executor> executor) {
    if (!static_cast<bool>(executor)) {
        throw std::invalid_argument(details::consts::k_timer_queue_make_delay_object_executor_null_err_msg);
    }

    return make_delay_object_impl(due_time, shared_from_this(), std::move(executor));
}

milliseconds timer_queue::max_worker_idle_time() const noexcept {
    return m_max_waiting_time;
}
