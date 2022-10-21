/*
    In this example we will collect all prime numbers from 0 to 1,000,000 in a parallel manner, using parallel coroutines.
 */

#include <cmath>
#include <iostream>

#include "concurrencpp/concurrencpp.h"

bool is_prime(int num) {
    if (num <= 1) {
        return false;
    }

    if (num <= 3) {
        return true;
    }

    const auto range = static_cast<int>(std::sqrt(num));
    if (num % 2 == 0 || num % 3 == 0) {
        return false;
    }

    for (int i = 5; i <= range; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) {
            return false;
        }
    }

    return true;
}

// runs in parallel
concurrencpp::result<std::vector<int>> find_primes(concurrencpp::executor_tag,
                                                   std::shared_ptr<concurrencpp::executor> executor,
                                                   int begin,
                                                   int end) {
    assert(begin <= end);

    std::vector<int> prime_numbers;
    for (int i = begin; i < end; i++) {
        if (is_prime(i)) {
            prime_numbers.emplace_back(i);
        }
    }

    co_return prime_numbers;
}

concurrencpp::result<std::vector<int>> find_prime_numbers(std::shared_ptr<concurrencpp::executor> executor) {
    const auto concurrency_level = executor->max_concurrency_level();
    const auto range_length = 1'000'000 / concurrency_level;

    std::vector<concurrencpp::result<std::vector<int>>> found_primes_in_range;
    int range_begin = 0;

    for (int i = 0; i < concurrency_level; i++) {
        auto result = find_primes({}, executor, range_begin, range_begin + range_length);
        range_begin += range_length;

        found_primes_in_range.emplace_back(std::move(result));
    }

    auto all_done = co_await concurrencpp::when_all(executor, found_primes_in_range.begin(), found_primes_in_range.end());

    std::vector<int> found_primes;
    for (auto& done_result : all_done) {
        auto prime_numbers = co_await done_result;
        found_primes.insert(found_primes.end(), prime_numbers.begin(), prime_numbers.end());
    }

    co_return found_primes;
}

int main(int argc, const char* argv[]) {
    concurrencpp::runtime runtime;
    auto all_prime_numbers = find_prime_numbers(runtime.thread_pool_executor()).get();

    for (auto i : all_prime_numbers) {
        std::cout << i << std::endl;
    }

    return 0;
}
