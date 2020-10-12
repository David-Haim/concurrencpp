#ifndef CONCURRENCPP_RESULT_GENERATOR_H
#define CONCURRENCPP_RESULT_GENERATOR_H

#include "fast_randomizer.h"

#include <string>

namespace concurrencpp::tests {

template<class type>
class result_generator {};

template<>
class result_generator<int> {

 private:
  random m_rand;

 public:
  int operator()() noexcept {
    return static_cast<int>(m_rand());
  }
};

template<>
class result_generator<std::string> {

 private:
  random m_rand;

 public:
  std::string operator()() noexcept {
    return std::string("123456#") + std::to_string(m_rand());
  }
};

}  // namespace concurrencpp::tests

#endif
