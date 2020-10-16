#ifndef CONCURRENCPP_PROXY_CORO_H
#define CONCURRENCPP_PROXY_CORO_H

#include "concurrencpp/concurrencpp.h"

namespace concurrencpp::tests {
template<class type>
class proxy_coro {

 private:
  result<type> m_result;
  std::shared_ptr<concurrencpp::executor> m_test_executor;
  bool m_force_rescheduling;

 public:
  proxy_coro(result<type> result,
             std::shared_ptr<concurrencpp::executor> test_executor,
             bool force_rescheduling) noexcept :
      m_result(std::move(result)),
      m_test_executor(std::move(test_executor)),
      m_force_rescheduling(force_rescheduling) {}

  result<type> operator()() {
    co_return co_await m_result.await_via(std::move(m_test_executor),
                                          m_force_rescheduling);
  }
};
}  // namespace concurrencpp::tests

#endif
