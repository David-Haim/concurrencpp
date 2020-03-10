#include "timer_queue.h"
#include "timer.h"

#include "../results/result.h"

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
                assert(existing_timer->is_oneshot()); //the timer was already deleted by the queue when it was fired.
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
                }
                else {
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

                auto first_timer_it = m_timers.begin(); //closest deadline
                auto timer_ptr = *first_timer_it;
                const auto is_oneshot = timer_ptr->is_oneshot();

                if (!timer_ptr->expired(now)) {
                    //if this timer is not expired, the next ones are guaranteed not to, as the set is ordered by deadlines.
                    break;
                }

                //we are going to modify the timer, so first we extract it
                auto timer_node = m_timers.extract(first_timer_it);

            	//we cannot use the naked node_handle according to the standard. it must be contained somewhere.
                auto temp_it = temp_set.insert(std::move(timer_node));

                (*temp_it)->fire();

                if (is_oneshot) {
                    m_iterator_mapper.erase(timer_ptr);
                    continue; //let the timer die inside the temp_set
                }

                //regular timer, re-insert into the right position
                timer_node = temp_set.extract(temp_it);
                auto new_it = m_timers.insert(std::move(timer_node));
                assert(m_iterator_mapper.contains(timer_ptr));
                m_iterator_mapper[timer_ptr] = new_it; //update the iterator map, multiset::extract invalidates the timer
            }

            if (m_timers.empty()) {
                reset_containers_memory();
                return now + std::chrono::hours(24);
            }

            //get the closest deadline.
            return (**m_timers.begin()).get_deadline();
        }
    };
}

timer_queue::timer_queue() noexcept : m_abort(false) {}

timer_queue::~timer_queue() noexcept {
    {
        std::unique_lock<decltype(m_lock)> lock(m_lock);
        if (!m_worker.joinable()) {
            return;
        }

        m_abort = true;
    }

    m_condition.notify_all();
    m_worker.join();
}

void timer_queue::add_timer(std::unique_lock<std::mutex>& lock, timer_ptr new_timer) noexcept {
    assert(lock.owns_lock());
    m_request_queue.emplace_back(std::move(new_timer), timer_request::add);
    lock.unlock();

    m_condition.notify_one();
}

void timer_queue::remove_timer(timer_ptr existing_timer) noexcept {
    {
        std::unique_lock<decltype(m_lock)> lock(m_lock);
        m_request_queue.emplace_back(std::move(existing_timer), timer_request::remove);
    }

    m_condition.notify_one();
}

void timer_queue::work_loop() noexcept {
    time_point next_deadline;
    details::timer_queue_internal internal_state;

    while (true) {
        std::unique_lock<decltype(m_lock)> lock(m_lock);
        if (internal_state.empty()) {
            m_condition.wait(lock, [this] {
                return !m_request_queue.empty() || m_abort;
            });
        }
        else {
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

void timer_queue::ensure_worker_thread(std::unique_lock<std::mutex>& lock) {
    (void)lock;
    assert(lock.owns_lock());
    if (m_worker.joinable()) {
        return;
    }

    m_worker = details::thread("concurrencpp::timer_queue worker", [this] {
        work_loop();
    });
}

concurrencpp::result<void> timer_queue::make_delay_object(size_t due_time, std::shared_ptr<executor> executor) {
    if (!static_cast<bool>(executor)) {
        throw std::invalid_argument("concurrencpp::timer_queue::make_delay_object - executor is null.");
    }

    concurrencpp::result_promise<void> promise;
    auto task = promise.get_result();

    make_timer_impl(
        due_time,
        0,
        std::move(executor),
        true,
        [tcs = std::move(promise)]() mutable { tcs.set_result(); });

    return task;
}