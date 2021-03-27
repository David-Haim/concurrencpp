#include "concurrencpp/concurrencpp.h"

#include <algorithm>
#include <iostream>

concurrencpp::result<void> quick_sort(concurrencpp::executor_tag,
                                      std::shared_ptr<concurrencpp::thread_pool_executor> tp,
                                      int* a,
                                      int lo,
                                      int hi);

int main() {
    std::cout << "Starting parallel quick sort test" << std::endl;

    concurrencpp::runtime_options opts;
    opts.max_cpu_threads = std::thread::hardware_concurrency() * 8;
    concurrencpp::runtime runtime(opts);

    ::srand(::time(nullptr));

    std::vector<int> array(8'000'000);
    for (auto& i : array) {
        i = ::rand() % 100'000;
    }

    quick_sort({}, runtime.thread_pool_executor(), array.data(), 0, array.size() - 1).get();

    const auto is_sorted = std::is_sorted(array.begin(), array.end());
    if (!is_sorted) {
        std::cerr << "Quick sort test failed: array is not sorted." << std::endl;
        std::abort();
    }

    std::cout << "================================" << std::endl;
}

using namespace concurrencpp;

/*

algorithm partition(A, lo, hi) is
        pivot := A[(hi + lo) / 2]
        i := lo - 1
        j := hi + 1
        loop forever
                do
                        i := i + 1
                while A[i] < pivot
                do
                        j := j - 1
                while A[j] > pivot
                if i ≥ j then
                        return j
                swap A[i] with A[j]
*/

int partition(int* a, int lo, int hi) {
    const auto pivot = a[(hi + lo) / 2];
    auto i = lo - 1;
    auto j = hi + 1;

    while (true) {
        do {
            ++i;
        } while (a[i] < pivot);

        do {
            --j;
        } while (a[j] > pivot);

        if (i >= j) {
            return j;
        }

        std::swap(a[i], a[j]);
    }
}

/*
                algorithm quicksort(A, lo, hi) is
                   if lo < hi then
                        p := partition(A, lo, hi)
                        quicksort(A, lo, p)
                        quicksort(A, p + 1, hi)

*/
result<void> quick_sort(executor_tag, std::shared_ptr<thread_pool_executor> tp, int* a, int lo, int hi) {
    if (lo >= hi) {
        co_return;
    }

    const auto p = partition(a, lo, hi);
    auto res0 = quick_sort({}, tp, a, lo, p);
    auto res1 = quick_sort({}, tp, a, p + 1, hi);

    co_await res0;
    co_await res1;
}
