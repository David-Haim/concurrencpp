#ifndef CONCURRENCPP_RESULT_TEST_EXECUTORS_H
#define CONCURRENCPP_RESULT_TEST_EXECUTORS_H

#include "concurrencpp/concurrencpp.h"

#include "result_factory.h"
#include "test_ready_result.h"

#include <type_traits>
#include <memory>

#include "../../helpers/random.h"

namespace concurrencpp::tests {
class test_executor : public ::concurrencpp::executor {

   private:
    mutable std::mutex m_lock;
    std::thread m_execution_thread;
    std::thread m_setting_thread;
    bool m_abort;

   public:
    test_executor() noexcept :
        executor("test_executor"), m_abort(false) {}

    ~test_executor() noexcept {
        shutdown();
    }

    int max_concurrency_level() const noexcept override {
        return 1;
    }

    bool shutdown_requested() const noexcept override {
        std::unique_lock<std::mutex> lock(m_lock);
        return m_abort;
    }

    void shutdown() noexcept override {
        std::unique_lock<std::mutex> lock(m_lock);
        if (m_abort) {
            return;
        }

        m_abort = true;

        if (m_execution_thread.joinable()) {
            m_execution_thread.join();
        }

        if (m_setting_thread.joinable()) {
            m_setting_thread.join();
        }
    }

    bool scheduled_async() const noexcept {
        std::unique_lock<std::mutex> lock(m_lock);
        return std::this_thread::get_id() == m_execution_thread.get_id();
    }

    bool scheduled_inline() const noexcept {
        std::unique_lock<std::mutex> lock(m_lock);
        return std::this_thread::get_id() == m_setting_thread.get_id();
    }

    void enqueue(std::experimental::coroutine_handle<> task) override {
        std::unique_lock<std::mutex> lock(m_lock);
        assert(!m_execution_thread.joinable());
        m_execution_thread = std::thread([task]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            task();
        });
    }

    void enqueue(std::span<std::experimental::coroutine_handle<>> span) override {
        (void)span;
        std::abort();  // not neeeded.
    }

    template<class type>
    void set_rp_value(result_promise<type> rp) {
        std::unique_lock<std::mutex> lock(m_lock);
        assert(!m_setting_thread.joinable());
        m_setting_thread = std::thread([rp = std::move(rp)]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            rp.set_from_function(result_factory<type>::get);
        });
    }

    template<class type>
    size_t set_rp_err(result_promise<type> rp) {
        random randomizer;
        const auto id = static_cast<size_t>(randomizer());

        std::unique_lock<std::mutex> lock(m_lock);
        assert(!m_setting_thread.joinable());
        m_setting_thread = std::thread([id, rp = std::move(rp)]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            rp.set_exception(std::make_exception_ptr(costume_exception(id)));
        });
        return id;
    }
};

inline std::shared_ptr<test_executor> make_test_executor() {
    return std::make_shared<test_executor>();
}
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
struct executor_enqueue_exception {};

struct throwing_executor : public concurrencpp::executor {
    throwing_executor() :
        executor("throwing_executor") {}

    void enqueue(std::experimental::coroutine_handle<>) override {
        throw executor_enqueue_exception();
    }

    void enqueue(std::span<std::experimental::coroutine_handle<>>) override {
        throw executor_enqueue_exception();
    }

    int max_concurrency_level() const noexcept override {
        return 0;
    }

    bool shutdown_requested() const noexcept override {
        return false;
    }

    void shutdown() noexcept override {
        // do nothing
    }
};

template<class type>
void test_executor_error_thrown(
    concurrencpp::result<type> result,
    std::shared_ptr<concurrencpp::executor> throwing_executor) {
    assert_equal(result.status(), concurrencpp::result_status::exception);
    try {
        result.get();
    } catch (concurrencpp::errors::executor_exception ex) {
        assert_equal(ex.throwing_executor.get(), throwing_executor.get());

        try {
            std::rethrow_exception(ex.thrown_exception);
        } catch (executor_enqueue_exception) {
            // OK
        } catch (...) {
            assert_false(true);
        }
    } catch (...) {
        assert_true(false);
    }
}
}  // namespace concurrencpp::tests

#endif  // CONCURRENCPP_RESULT_HELPERS_H
