#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <array>
#include <random>
#include <functional>

concurrencpp::result<void> test_matrix_multiplication(
    std::shared_ptr<concurrencpp::thread_pool_executor> ex);

int main() {
  concurrencpp::runtime runtime;
  test_matrix_multiplication(runtime.thread_pool_executor()).get();
}

using namespace concurrencpp;

using matrix = std::array<std::array<double, 1024>, 1024>;

std::unique_ptr<matrix> make_matrix() {
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(-1000.0, 1000.0);
  auto mtx_ptr = std::make_unique<matrix>();
  auto& mtx = *mtx_ptr;

  for (size_t i = 0; i < 1024; i++) {
    for (size_t j = 0; j < 1024; j++) {
      mtx[i][j] = distribution(generator);
    }
  }

  return mtx_ptr;
}

void test_matrix(const matrix& mtx0, const matrix& mtx1, const matrix& mtx2) {
  for (size_t i = 0; i < 1024; i++) {
    for (size_t j = 0; j < 1024; j++) {

      double res = 0.0;

      for (size_t k = 0; k < 1024; k++) {
        res += mtx0[i][k] * mtx1[k][j];
      }

      if (mtx2[i][j] != res) {
        std::cerr << "matrix multiplication test failed. expected " << res
                  << " got " << mtx2[i][j] << "at matix position[" << i << ","
                  << j << std::endl;
      }
    }
  }
}

result<void> do_multiply(executor_tag,
                         std::shared_ptr<thread_pool_executor> executor,
                         const matrix& mtx0,
                         const matrix& mtx1,
                         matrix& mtx2,
                         size_t line,
                         size_t col) {

  double res = 0.0;
  for (size_t i = 0; i < 1024; i++) {
    res += mtx0[line][i] * mtx1[i][col];
  }

  mtx2[line][col] = res;
  co_return;
};

result<void> test_matrix_multiplication(
    std::shared_ptr<thread_pool_executor> ex) {
  auto mtx0_ptr = make_matrix();
  auto mtx1_ptr = make_matrix();
  auto mtx2_ptr = std::make_unique<matrix>();

  matrix& mtx0 = *mtx0_ptr;
  matrix& mtx1 = *mtx1_ptr;
  matrix& mtx2 = *mtx2_ptr;

  std::vector<result<void>> results;
  results.reserve(1024 * 1024);

  for (size_t i = 0; i < 1024; i++) {
    for (size_t j = 0; j < 1024; j++) {
      results.emplace_back(do_multiply({}, ex, mtx0, mtx1, mtx2, i, j));
    }
  }

  for (auto& result : results) {
    co_await result;
  }

  test_matrix(mtx0, mtx1, mtx2);
}