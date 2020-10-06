#ifndef CONCURRENCPP_THREAD_HELPERS_H
#define CONCURRENCPP_THREAD_HELPERS_H

#include "concurrencpp/concurrencpp.h"

#include <memory>
#include <atomic>

#include <iostream>

namespace concurrencpp::tests {
class test_listener {

 private:
  std::atomic_size_t m_total_created;
  std::atomic_size_t m_total_destroyed;
  std::atomic_size_t m_total_waiting;
  std::atomic_size_t m_total_resuming;
  std::atomic_size_t m_total_idling;

 public:
  test_listener() noexcept :
      m_total_created(0), m_total_destroyed(0), m_total_waiting(0),
      m_total_resuming(0), m_total_idling(0) {}

  virtual ~test_listener() noexcept = default;

  size_t total_created() const noexcept {
    return m_total_created.load();
  }
  size_t total_destroyed() const noexcept {
    return m_total_destroyed.load();
  }
  size_t total_waiting() const noexcept {
    return m_total_waiting.load();
  }
  size_t total_resuming() const noexcept {
    return m_total_resuming.load();
  }
  size_t total_idling() const noexcept {
    return m_total_idling.load();
  }

  /*
  virtual void on_thread_created(std::thread::id id) override {
          ++m_total_created;
  }

  virtual void on_thread_waiting(std::thread::id id) override {
          ++m_total_waiting;
  }

  virtual void on_thread_resuming(std::thread::id id) override {
          ++m_total_resuming;
  }

  virtual void on_thread_idling(std::thread::id id) override {
          ++m_total_idling;
  }

  virtual void on_thread_destroyed(std::thread::id id) override {
          ++m_total_destroyed;
  }

  void reset() {
          m_total_created = 0;
          m_total_destroyed = 0;
          m_total_waiting = 0;
          m_total_resuming = 0;
          m_total_idling = 0;
  }
  */
};

inline std::shared_ptr<test_listener> make_test_listener() {
  return std::make_shared<test_listener>();
}

class waiter {

  class waiter_impl {

   private:
    std::mutex m_lock;
    std::condition_variable m_cv;
    bool m_unblocked;

   public:
    waiter_impl() : m_unblocked(false) {}

    ~waiter_impl() {
      notify_all();
    }

    void wait() {
      std::unique_lock<decltype(m_lock)> lock(m_lock);
      m_cv.wait(lock, [this] {
        return m_unblocked;
      });

      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    void notify_all() {
      std::unique_lock<decltype(m_lock)> lock(m_lock);
      m_unblocked = true;
      m_cv.notify_all();
    }
  };

 private:
  const std::shared_ptr<waiter_impl> m_impl;

 public:
  waiter() : m_impl(std::make_shared<waiter_impl>()) {}

  waiter(const waiter&) noexcept = default;

  void wait() {
    m_impl->wait();
  }

  void notify_all() {
    m_impl->notify_all();
  }
};
}  // namespace concurrencpp::tests

#endif