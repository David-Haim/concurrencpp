#include "concurrencpp/concurrencpp.h"

#include <iostream>

concurrencpp::result<int> fibbonacci(
    concurrencpp::executor_tag,
    std::shared_ptr<concurrencpp::thread_pool_executor> tpe,
    int curr);
int fibbonacci_sync(int i);

int main() {
  concurrencpp::runtime_options opts;
  opts.max_cpu_threads = 24;
  concurrencpp::runtime runtime(opts);

  auto fibb = fibbonacci(concurrencpp::executor_tag {},
                         runtime.thread_pool_executor(), 32)
                  .get();
  auto fibb_sync = fibbonacci_sync(32);
  if (fibb != fibb_sync) {
    std::cerr << "fibonnacci test failed. expected " << fibb_sync << " got "
              << fibb << std::endl;
    std::abort();
  }

  std::cout << fibb << std::endl;
}

using namespace concurrencpp;

result<int> fibbonacci_split(std::shared_ptr<thread_pool_executor> tpe,
                             const int curr) {
  auto fib_1 = fibbonacci(executor_tag {}, tpe, curr - 1);
  auto fib_2 = fibbonacci(executor_tag {}, tpe, curr - 2);

  co_return co_await fib_1 + co_await fib_2;
}

result<int> fibbonacci(executor_tag,
                       std::shared_ptr<thread_pool_executor> tpe,
                       const int curr) {
  if (curr == 1) {
    co_return 1;
  }

  if (curr == 0) {
    co_return 0;
  }

  co_return co_await fibbonacci_split(tpe, curr);
}

int fibbonacci_sync(int i) {
  if (i == 0) {
    return 0;
  }

  if (i == 1) {
    return 1;
  }

  return fibbonacci_sync(i - 1) + fibbonacci_sync(i - 2);
}
