# concurrencpp, the C++ concurrency library

![Latest Release](https://img.shields.io/github/v/release/David-Haim/concurrencpp.svg) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

concurrencpp is a tasking library for C++ allowing developers to write highly concurrent applications easily and safely by using tasks, executors and coroutines.
By using concurrencpp applications can break down big procedures that need to be processed asynchronously into smaller tasks that run concurrently and work in a co-operative manner to achieve the wanted result.
concurrencpp also allows applications to write parallel algorithms easily by using parallel coroutines.

concurrencpp main advantages are:
* Being able to write modern concurrency code without having to rely on low-level concurrency primitives like locks and condition variables.
* Being able to write highly concurrent and parallel applications that scale automatically to use all hardware resources, as needed.
* Being able to write non-blocking, synchronous-like code easily by using   C++20 coroutines and the `co_await` keyword.
* Reducing the possibility of race conditions, data races and deadlocks by using high-level objects with built-in synchronization.
* concurrencpp provides various types of commonly used executors with a complete coroutine integration.
* Applications can extend the library by implementing their own provided executors.
----
 ### Table of contents
* [concurrencpp overview](#concurrencpp-overview)
* [Tasks](#tasks)
    * [concurrencpp coroutines](#concurrencpp-coroutines )
* [Executors](#executors)
    * [`executor` API](#executor-api)
    * [Executor types](#executor-types)
    * [Using executors](#using-executors)
    * [`thread_pool_executor` API](#thread_pool_executor-api)
    * [`manual_executor` API](#manual_executor-api)
* [Result objects](#result-objects)
    * [`result` API](#result-api)
* [Parallel coroutines](#parallel-coroutines)
    * [Parallel Fibonacci example](#parallel-fibonacci-example)
* [Result-promises](#result-promises)
    * [`result_promise` API](#result_promise-api)
    * [`result_promise` example](#example-marshaling-asynchronous-result-using-result_promise)
* [Shared result objects](#shared-result-objects)
    * [`shared_result` API](#shared_result-api)
    * [`shared_result` example](#shared_result-example)
* [Summery: using tasks and coroutines](#summery-using-tasks-and-coroutines)
* [Result auxiliary functions](#result-auxiliary-functions)
* [Timers and Timer queues](#timers-and-timer-queues)
    * [`timer_queue` API](#timer_queue-api)
    * [`timer` API](#timer-api)
    * [Regular timer example](#regular-timer-example)
    * [Oneshot timers](#oneshot-timers)
    * [Oneshot timer example](#oneshot-timer-example)
    * [Delay objects](#delay-objects)
    * [Delay object example](#delay-object-example)
* [The runtime object](#the-runtime-object)
    * [`runtime` API](#runtime-api)
    * [Creating user-defined executors](#creating-user-defined-executors)
    * [`task` API](#task-api)
    * [Using a user-defined executor example](#example-using-a-user-defined-executor)
* [Supported platforms and tools](#supported-platforms-and-tools)
* [Building, installing and testing](#building-installing-and-testing)

----

###  concurrencpp overview

concurrencpp is a task-centric library. A task is an asynchronous operation. Tasks offer a higher level of abstraction for concurrent code than traditional thread-centric approaches. Tasks can be chained together, meaning that tasks pass their asynchronous result from one to another, where the result of one task is used as if it were a parameter or an intermediate value of another ongoing task. Tasks allow applications to utilize available hardware resources better and scale much more than using raw threads, since tasks can be suspended, waiting for another task to produce a result, without blocking underlying OS-threads. Tasks bring much more productivity to developers by allowing them to focus more on business-logic and less on low-level concepts like thread management and inter-thread synchronization.

While tasks specify *what* actions have to be executed, *executors* are worker-objects that specify *where and how* to execute tasks. Executors spare applications the managing of thread pools and task queues themselves. Executors also decouple those concepts away from application code, by providing a unified API for creating and scheduling tasks.

Tasks communicate with each other using *result objects*. A result object is an asynchronous pipe that pass the asynchronous result of one task to another ongoing-task. Results can be awaited and resolved in a non-blocking manner.

These 3 concepts - the task, the executor and the associated result are the building blocks of concurrencpp. Executors run tasks that communicate with each-other by sending results through result-objects. Tasks, executors and result objects work together symbiotically to produce concurrent code which is fast and clean.

concurrencpp is built around the RAII concept. In order to use tasks and executors, applications create a `runtime` instance in the beginning of the `main` function. The runtime is then used to acquire existing executors and register new user-defined executors. Executors are used to create and schedule tasks to run, and they might return a `result` object that can be used to marshal the asynchronous result to another task that acts as its consumer.
When the runtime is destroyed, it iterates over every stored executor and calls its `shutdown` method. Every executor then exits gracefully. Unscheduled tasks are destroyed, and attempts to create new tasks will throw an exception.

#### *"Hello world" program using concurrencpp:*

```cpp
#include "concurrencpp/concurrencpp.h"
#include <iostream>

int main() {
    concurrencpp::runtime runtime;
    auto result = runtime.thread_executor()->submit([] {
        std::cout << "hello world" << std::endl;
    });

    result.get();
    return 0;
}
```

In this basic example, we created a runtime object, then we acquired the thread executor from the runtime. We used `submit` to pass a lambda as our given callable. This lambda returns `void`, hence, the executor returns a `result<void>` object that marshals the asynchronous result back to the caller.  `main` calls  `get` which blocks the main thread until the result becomes ready. If no exception was thrown, `get` returns `void`. If an exception was thrown, `get` re-throws it. Asynchronously, `thread_executor` launches a new thread of execution and runs the given lambda. It implicitly `co_return void` and the task is finished. `main` is then unblocked.
      
#### *Concurrent even-number counting:*

```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <vector>
#include <algorithm>

#include <ctime>

using namespace concurrencpp;

std::vector<int> make_random_vector() {
    std::vector<int> vec(64 * 1'024);

    std::srand(std::time(nullptr));
    for (auto& i : vec) {
        i = ::rand();
    }

    return vec;
}

result<size_t> count_even(std::shared_ptr<thread_pool_executor> tpe, const std::vector<int>& vector) {
    const auto vecor_size = vector.size();
    const auto concurrency_level = tpe->max_concurrency_level();
    const auto chunk_size = vecor_size / concurrency_level;

    std::vector<result<size_t>> chunk_count;

    for (auto i = 0; i < concurrency_level; i++) {
        const auto chunk_begin = i * chunk_size;
        const auto chunk_end = chunk_begin + chunk_size;
        auto result = tpe->submit([&vector, chunk_begin, chunk_end]() -> size_t {
            return std::count_if(vector.begin() + chunk_begin, vector.begin() + chunk_end, [](auto i) {
                return i % 2 == 0;
            });
        });

        chunk_count.emplace_back(std::move(result));
    }

    size_t total_count = 0;

    for (auto& result : chunk_count) {
        total_count += co_await result;
    }

    co_return total_count;
}

int main() {
    concurrencpp::runtime runtime;
    const auto vector = make_random_vector();
    auto result = count_even(runtime.thread_pool_executor(), vector);
    const auto total_count = result.get();
    std::cout << "there are " << total_count << " even numbers in the vector" << std::endl;
    return 0;
}
```

In this example, we start the program by creating a runtime object. We create a vector filled with random numbers, then we acquire the `thread_pool_executor` from the runtime and call `count_even`.
`count_even` is a coroutine that spawns more tasks and `co_await`s for them to finish inside.
`max_concurrency_level` returns the maximum amount of workers that the executor supports, In the threadpool executor case, the number of workers is calculated from the number of cores.
We then partition the array to match the number of workers and send every chunk to be processed in its own task.
Asynchronously, the workers count how many even numbers each chunk contains, and `co_return` the result.
`count_even` sums every result by pulling the count using `co_await`, the final result is then `co_return`ed.
The main thread, which was blocked by calling `get` is unblocked and the total count is returned.
main prints the number of even numbers and the program terminates gracefully.    

###  Tasks
Every big or complex operation can be broken down to smaller and chainable steps.
Tasks are asynchronous operations implementing those computational steps. Tasks can run anywhere with the help of executors. While tasks can be created from regular callables (such as functors and lambdas), Tasks are mostly used with coroutines, which allow smooth suspension and resumption. In concurrencpp, the task concept is represented by the `concurrencpp::task` class. Although the task concept is central to concurrenpp, applications will rarely have to create and manipulate task objects themselves, as task objects are created and scheduled by the runtime with no external help.   

####  concurrencpp coroutines

concurrencpp allows applications to produce and consume coroutines as the main way of creating tasks. concurrencpp coroutines are eager and start to run the moment they are invoked (as opposed to lazy coroutines, which start to run only when `co_await`ed). concurrencpp coroutines can return any of `concurrencpp::result` or `concurrencpp::null_result`.

`concurrencpp::result` tells the coroutine to marshal the returned value or the thrown exception while `concurrencpp::null_result` tells the coroutine to drop and ignore any of them.
 
When a function returns any of `concurrencpp::result` or `concurrencpp::null_result`and contains at least one `co_await` or `co_return` in it's body, the function is a concurrencpp coroutine. Every valid concurrencpp coroutine is a valid task. In our count-even example above, `count_even` is such a coroutine. We first spawned `count_even`, then inside it the threadpool executor spawned more child tasks (that are created from regular callables),  that were eventually joined using `co_await`.

Coroutines can start to run synchronously, in the caller thread. This kind of coroutines is called "regular coroutines".
Concurrencpp coroutines can also start to run in parallel, inside a given executor, this kind of coroutines is called "parallel coroutines".

### Executors

A concurrencpp executor is an object that is able to schedule and run tasks.
Executors simplify the work of managing resources such as threads, thread pools and task queues by decoupling them away from application code.
Executors provide a unified way of scheduling and executing tasks, since they all extend `concurrencpp::executor`.

#### `executor` API

```cpp
class executor {
    /*
        Initializes a new executor and gives it a name.
    */
    executor(std::string_view name);

    /*
        Destroys this executor.
    */
    virtual ~executor() noexcept = default;

    /*
        The name of the executor, used for logging and debugging.
    */
    const std::string name;

    /*
        Schedules a task to run in this executor.
        Throws concurrencpp::errors::runtime_shutdown exception if shutdown was called before.
    */
    virtual void enqueue(concurrencpp::task task) = 0;

    /*
        Schedules a range of tasks to run in this executor.
        Throws concurrencpp::errors::runtime_shutdown exception if shutdown was called before.
    */    
    virtual void enqueue(std::span<concurrencpp::task> tasks) = 0;

    /*
        Returns the maximum count of real OS threads this executor supports.
        The actual count of threads this executor is running might be smaller than this number.
        returns numeric_limits<int>::max if the executor does not have a limit for OS threads.
    */
    virtual int max_concurrency_level() const noexcept = 0;

    /*
        Returns true if shutdown was called before, false otherwise.
    */
    virtual bool shutdown_requested() const noexcept = 0;

    /*
        Shuts down the executor:
        - Tells underlying threads to exit their work loop and joins them.
        - Destroys unexecuted coroutines.
        - Makes subsequent calls to enqueue, post, submit, bulk_post and
            bulk_submit to throw concurrencpp::errors::runtime_shutdown exception.
        - Makes shutdown_requested return true.
    */
    virtual void shutdown() noexcept = 0;

    /*
        Turns a callable and its arguments into a task object and schedules it to run in this executor using enqueue.
        Arguments are passed to the task by decaying them first.
         Throws errors::runtime_shutdown exception if shutdown has been called before.
    */
    template<class callable_type, class ... argument_types>
    void post(callable_type&& callable, argument_types&& ... arguments);
    
    /*
        Like post, but returns a result object that marshals the asynchronous result.
        Throws errors::runtime_shutdown exception if shutdown has been called before.
    */
    template<class callable_type, class ... argument_types>
    result<type> submit(callable_type&& callable, argument_types&& ... arguments);

    /*
        Turns an array of callables into an array of tasks and schedules them to run in this executor using enqueue.
        Throws errors::runtime_shutdown exception if shutdown has been called before.
    */
    template<class callable_type>
    void bulk_post(std::span<callable_type> callable_list);

    /*
        Like bulk_post, but returns an array of result objects that marshal the asynchronous results.
        Throws errors::runtime_shutdown exception if shutdown has been called before.
    */    
    template<class callable_type>
    std::vector<concurrencpp::result<type>> bulk_submit(std::span<callable_type> callable_list);
};
```

#### Executor types

As mentioned above, concurrencpp provides commonly used executors. These executor types are:

* **thread pool executor** - a general purpose executor that maintains a pool of threads.
The thread pool executor is suitable for short cpu-bound tasks that don't block. Applications are encouraged to use this executor as the default executor for non-blocking tasks.
The concurrencpp thread pool provides dynamic thread injection and dynamic work balancing.

* **background executor** - a threadpool executor with a larger pool of threads. Suitable for launching short blocking tasks like file io and db queries.

* **thread executor** - an executor that launches each enqueued task to run on a new thread of execution. Threads are not reused.
This executor is good for long running tasks, like objects that run a work loop, or long blocking operations.

* **worker thread executor** - a single thread executor that maintains a single task queue. Suitable when applications want a dedicated thread that executes many related tasks.

* **manual executor** - an executor that does not execute coroutines by itself. Application code can execute previously enqueued tasks by manually invoking its execution methods.

* **derivable executor** - a base class for user defined executors. Although inheriting  directly from `concurrencpp::executor` is possible, `derivable_executor` uses the `CRTP` pattern that provides some optimization opportunities for the compiler.
 
* **inline executor** - mainly used to override the behavior of other executors. Enqueuing a task is equivalent to invoking it inline.

#### Using executors

The bare mechanism of an executor is encapsulated in its `enqueue` method.
This method enqueues a task for execution and has two overloads:
One overload receives a single task object as an argument, and another that receives a span of task objects.
The second overload is used to enqueue a batch of tasks. This allows better scheduling heuristics and decreased contention.

Applications don't have to rely on `enqueue` alone, `concurrencpp::executor` provides an API for scheduling user callables by converting them to task objects behind the scenes.
Applications can request executors to return a result object that marshals the asynchronous result of the provided callable. This is done by calling `executor::submit` and `executor::bulk_submit`.
`submit` gets a callable, and returns a result object. `executor::bulk_submit` gets a `span` of callables and returns a `vector`of result objects in a similar way `submit` works.
In many cases, applications are not interested in the asynchronous value or exception. In this case, applications can use `executor:::post` and `executor::bulk_post` to schedule a callable or a `span` of callables to be executed, but also tells the task to drop any returned value or thrown exception. Not marshaling the asynchronous result is faster than marshaling, but then we have no way of knowing the status or the result of the ongoing task.

`post`, `bulk_post`, `submit` and `bulk_submit` use `enqueue` behind the scenes for the underlying scheduling mechanism.


#### `thread_pool_executor` API

Aside from `post`, `submit`, `bulk_post` and `bulk_submit`, the `thread_pool_executor`  provides additional methods.  

```cpp
class thread_pool_executor {

    /*
        Returns the number of milliseconds each thread-pool worker remains idle (without any task to execute) before exiting.
        This constant can be set by passing a runtime_options object to the constructor of the runtime class.
    */
    std::chrono::milliseconds max_worker_idle_time() const noexcept;

};
```
#### `manual_executor` API

Aside from `post`, `submit`, `bulk_post` and `bulk_submit`, the `manual_executor`  provides additional methods.

```cpp
class manual_executor {

    /*
        Destructor. Equivalent to clear.
    */
    ~manual_executor() noexcept;

    /*
        Returns the number of enqueued tasks at the moment of invocation.
        This number can change quickly by the time the application handles it, it should be used as a hint.
        This method is thread safe.
    */
    size_t size() const noexcept;
        
    /*
        Queries whether the executor is empty from tasks at the moment of invocation.
        This value can change quickly by the time the application handles it, it should be used as a hint.
        This method is thread safe.
    */
    bool empty() const noexcept;

    /*
        Clears the executor from any enqueued but yet to-be-executed tasks, and returns the number of cleared tasks.
        Tasks enqueued to this executor by (post_)submit method are resumed and errors::broken_task exception is thrown inside them.
        Ongoing tasks that are being executed by loop_once(_XXX) or loop(_XXX) are uneffected.
        This method is thread safe.
    */
    size_t clear();

    /*
        Tries to execute a single task. If at the moment of invocation the executor is empty, the method does nothing.
        Returns true if a task was executed, false otherwise.
        This method is thread safe.
    */
    bool loop_once();

    /*
        Tries to execute a single task.
        This method returns when either a task was executed or max_waiting_time (in milliseconds) has reached.
        If max_waiting_time is 0, the method is equivalent to loop_once.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    bool loop_once_for(std::chrono::milliseconds max_waiting_time);

    /*
        Tries to execute a single task.
        This method returns when either a task was executed or timeout_time has reached.
        If timeout_time has already expired, this method is equivalent to loop_once.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    template<class clock_type, class duration_type>
    bool loop_once_until(std::chrono::time_point<clock_type, duration_type> timeout_time);
   
    /*
        Tries to execute max_count enqueued tasks and returns the number of tasks that were executed.
        This method does not wait: it returns when the executor becomes empty from tasks or max_count tasks have been executed.
        This method is thread safe.
    */
    size_t loop(size_t max_count);

    /*
        Tries to execute max_count tasks.
        This method returns when either max_count tasks were executed or a total amount of max_waiting_time has passed.
        If max_waiting_time is 0, the method is equivalent to loop.
        Returns the actual amount of tasks that were executed.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    size_t loop_for(size_t max_count, std::chrono::milliseconds max_waiting_time);

    /*    
        Tries to execute max_count tasks.
        This method returns when either max_count tasks were executed or timeout_time has reached.
        If timeout_time has already expired, the method is equivalent to loop.
        Returns the actual amount of tasks that were executed.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    template<class clock_type, class duration_type>
    size_t loop_until(size_t max_count, std::chrono::time_point<clock_type, duration_type> timeout_time);
    
    /*
        Waits for at least one task to be available for execution.
        This method should be used as a hint, as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    void wait_for_task();

    /*
        This method returns when one or more tasks are available for execution or max_waiting_time has passed.    
        Returns true if at at least one task is available for execution, false otherwise.
        This method should be used as a hint, as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    bool wait_for_task_for(std::chrono::milliseconds max_waiting_time);

    /*
        This method returns when one or more tasks are available for execution or timeout_time has reached.    
        Returns true if at at least one task is available for execution, false otherwise.
        This method should be used as a hint, as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    template<class clock_type, class duration_type>
    bool wait_for_task_until(std::chrono::time_point<clock_type, duration_type> timeout_time);
    
    /*
        This method returns when max_count or more tasks are available for execution.    
        This method should be used as a hint, as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.
    */
    void wait_for_tasks(size_t max_count);

    /*
        This method returns when max_count or more tasks are available for execution or max_waiting_time (in milliseconds) has passed.    
        Returns the number of tasks available for execution when the method returns.
        This method should be used as a hint, as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.    
    */
    size_t wait_for_tasks_for(size_t count, std::chrono::milliseconds max_waiting_time);

    /*
        This method returns when max_count or more tasks are available for execution or timeout_time is reached.    
        Returns the number of tasks available for execution when the method returns.
        This method should be used as a hint, as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns and throws errors::shutdown_exception.
        This method is thread safe.    
    */
    template<class clock_type, class duration_type>
    size_t wait_for_tasks_until(size_t count, std::chrono::time_point<clock_type, duration_type> timeout_time);
        
};
```

### Result objects

Asynchronous values and exceptions can be consumed using the concurrencpp result objects.
A result object is a pipe for the asynchronous result, like `std::future`.
When a task finishes execution, it either returns a valid value or throws an exception.
In either case, this asynchronous result is marshaled to the consumer of the result object.
The result status therefore, varies from `idle` (the asynchronous result or exception aren't ready yet) to `value` (the task terminated by returning a valid value) to `exception` (the task terminated by throwing an exception).  

Result objects are a move-only type, and as such, they cannot be used after their content was moved to another result object. In this case, the result object is considered to be empty and attempts to call any method other than `operator bool` and `operator = ` will throw.
After the asynchronous result has been pulled out of the result object (by calling `get`, `await` or `await_via`), the result object becomes empty. Emptiness can be tested with `operator bool`.

Results can be polled, waited, awaited or resolved.

Result objects can be polled for their status by calling `result::status`.

Results can be waited by calling any of `result::wait`, `result::wait_for`, `result::wait_until` or `result::get`.
Waiting a result is a blocking operation (in the case the asynchronous result is not ready), and will suspend the entire thread of execution waiting for the asynchronous result to become available. Waiting operations are generally discouraged and only allowed in root-level tasks or in contexts which allow it, like blocking the main thread waiting for the rest of the application to finish gracefully, or using `concurrencpp::blocking_executor` or `concurrencpp::thread_executor`.

Awaiting a result means to suspend the current coroutine until the asynchronous result is ready. If a valid value was returned from the associated task, it is returned from the result object. If the associated task throws an exception, it is re-thrown.
At the moment of awaiting, if the result is already ready, the current coroutine resumes immediately. Otherwise, it is resumed by the thread that sets the asynchronous result or exception.

The behavior of awaiting result objects can be further fine tuned by using `await_via`.
This method accepts an executor and a boolean flag (`force_rescheduling`).
If, at the moment of awaiting, the result is already ready, the behavior depends on the value of `force_rescheduling`.
If `force_rescheduling` is true, the current coroutine is forcefully suspended and resumed inside the given executor.
If `force_rescheduling` is false, the current coroutine is resumed immediately in the calling thread.
If the asynchronous result is not ready at the moment of awaiting, the current coroutine resumes after the result is set, by scheduling it to run in the given executor.

Resolving a result is similar to awaiting it. The different is that the `co_await` expression will return the result object itself,
in a non empty form, in a ready state. The asynchronous result can then be pulled by using `get` or `co_await`.
Just like `await_via`, `resolve_via` fine tunes the control flow of the coroutine by passing an executor and a flag suggesting how to behave when the result is already ready.

Awaiting a result object by using `co_await` (and by doing so, turning the current function/task into a coroutine as well) is the preferred way of consuming result objects, as it does not block underlying threads.

#### `result` API
    
```cpp
class result{
    /*
        Creates an empty result that isn't associated with any task.
    */
    result() noexcept = default;

    /*
        Destroys the result. Associated tasks are not cancelled.
        The destructor does not block waiting for the asynchronous result to become ready.
    */    
    ~result() noexcept = default;

    /*
        Moves the content of rhs to *this. After this call, rhs is empty.
    */
    result(result&& rhs) noexcept = default;

    /*
        Moves the content of rhs to *this. After this call, rhs is empty. Returns *this.        
    */
    result& operator = (result&& rhs) noexcept = default;

    /*
        Returns true if this is a non-empty result.
        Applications must not use this object if this->operator bool() is false.
    */
    explicit operator bool() const noexcept;

    /*
        Queries the status of *this.
        The return value is any of result_status::idle, result_status::value or result_status::exception.
        Throws concurrencpp::errors::empty_result if *this is empty.        
    */
    result_status status() const;

    /*
        Blocks the current thread of execution until this result is ready, when status() != result_status::idle.
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    void wait();

    /*
        Blocks until this result is ready or duration has passed. Returns the status of this result after unblocking.
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    template<class duration_unit, class ratio>
    result_status wait_for(std::chrono::duration<duration_unit, ratio> duration);

    /*
        Blocks until this result is ready or timeout_time has reached. Returns the status of this result after unblocking.
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    template< class clock, class duration >
    result_status wait_until(std::chrono::time_point<clock, duration> timeout_time);

    /*
        Blocks the current thread of execution until this result is ready, when status() != result_status::idle.
        If the result is a valid value, it is returned, otherwise, get rethrows the asynchronous exception.        
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    type get();

    /*
        Returns an awaitable used to await this result.
        If the result is already ready - the current coroutine resumes immediately in the calling thread of execution.
        If the result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready,
        by the thread which had set the asynchronous value or exception.
        In either way, after resuming, if the result is a valid value, it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws concurrencpp::errors::empty_result if *this is empty.                            
    */
    auto operator co_await();

    /*
        Returns an awaitable used to await this result.
        If the result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready,
        by scheduling the current coroutine via executor.
        If the result is already ready - the behaviour depends on the value of force_rescheduling:
            If force_rescheduling = true, then the current coroutine is forcefully suspended and resumed via executor.
            If force_rescheduling = false, then the current coroutine resumes immediately in the calling thread of execution.
        In either way, after resuming, if the result is a valid value, it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws concurrencpp::errors::empty_result if *this is empty.        
        Throws std::invalid_argument if executor is null.
        If this result is ready and force_rescheduling=true, throws any exception that executor::enqueue may throw.    
    */
    auto await_via(
        std::shared_ptr<concurrencpp::executor> executor,
        bool force_rescheduling = true);

    /*
        Returns an awaitable used to resolve this result.
        After co_await expression finishes, *this is returned in a non-empty form, in a ready state.
        Throws concurrencpp::errors::empty_result if *this is empty.
    */    
    auto resolve();

    /*
        Returns an awaitable used to resolve this result.
        If the result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready,
        by scheduling the current coroutine via executor.
        If the result is already ready - the behaviour depends on the value of force_rescheduling:
            If force_rescheduling = true, then the current coroutine is forcefully suspended and resumed via executor.
            If force_rescheduling = false, then the current coroutine resumes immediately in the calling thread of execution.
        In either way, after resuming, *this is returned in a non-empty form and guaranteed that its status is not result_status::idle.
        Throws concurrencpp::errors::empty_result if *this is empty.        
        Throws std::invalid_argument if executor is null.
        If this result is ready and force_rescheduling=true, throws any exception that executor::enqueue may throw.                    
    */
    auto resolve_via(
        std::shared_ptr<concurrencpp::executor> executor,
        bool force_rescheduling = true);
};
```

###  Parallel coroutines

Regular coroutines start to run synchronously in the calling thread of execution. Execution might shift to another thread of execution if the coroutine undergoes a rescheduling, for example by awaiting an unready result object inside it.
concurrencpp also provides parallel coroutines, which start to run inside a given executor, not in the invoking thread of execution. This style of scheduling coroutines is especially helpful when writing parallel algorithms, recursive algorithms and concurrent algorithms that use the fork-join model.

Every parallel coroutine must meet the following preconditions:

1. Returns any of `result` / `null_result` .
1. Gets `executor_tag` as its first argument .
1. Gets any of `type*` / `type&` / `std::shared_ptr<type>`, where `type` is a concrete class of `executor` as its second argument.
1. Contains any of `co_await` or `co_return` in its body.

If all the above applies, the function is a parallel coroutine:
concurrencpp will start the coroutine suspended and immediately reschedule it to run in the provided executor.
`concurrencpp::executor_tag` is a dummy placeholder to tell the concurrencpp runtime that this function is not a regular function, it needs to start running inside the given executor.
Applications can then consume the result of the parallel coroutine by using the returned result object.

#### *Parallel Fibonacci example:*
```cpp
#include "concurrencpp/concurrencpp.h"
#include <iostream>

using namespace concurrencpp;

int fibonacci_sync(int i) {
    if (i == 0) {
        return 0;
    }

    if (i == 1) {
        return 1;
    }

    return fibonacci_sync(i - 1) + fibonacci_sync(i - 2);
}

result<int> fibonacci(executor_tag, std::shared_ptr<thread_pool_executor> tpe, const int curr) {
    if (curr <= 10) {
        co_return fibonacci_sync(curr);
    }

    auto fib_1 = fibonacci({}, tpe, curr - 1);
    auto fib_2 = fibonacci({}, tpe, curr - 2);

    co_return co_await fib_1 + co_await fib_2;
}

int main() {
    concurrencpp::runtime runtime;
    auto fibb_30 = fibonacci({}, runtime.thread_pool_executor(), 30).get();
    std::cout << "fibonacci(30) = " << fibb_30 << std::endl;
    return 0;
}
```

In this example, we calculate the 30-th member of the Fibonacci sequence in a parallel manner.
We start launching each Fibonacci step in its own parallel coroutine. The first argument is a dummy `executor_tag` and the second argument is the threadpool executor.
Every recursive step invokes a new parallel coroutine that runs in parallel. Each result is `co_return`ed to its parent task and acquired by using `co_await`.   
When we deem the input to be small enough to be calculated synchronously (when `curr <= 10`), we stop executing each recursive step in its own task and just solve the algorithm synchronously.

To compare, this is how the same code is written without using parallel coroutines, and relying on `executor::submit` alone.
Since `fibonacci` returns a `result<int>`, submitting it recursively via `executor::submit` will result a `result<result<int>>`.

```cpp
#include "concurrencpp/concurrencpp.h"
#include <iostream>

using namespace concurrencpp;

int fibonacci_sync(int i) {
    if (i == 0) {
        return 0;
    }

    if (i == 1) {
        return 1;
    }

    return fibonacci_sync(i - 1) + fibonacci_sync(i - 2);
}

result<int> fibonacci(std::shared_ptr<thread_pool_executor> tpe, const int curr) {
    if (curr <= 10) {
        co_return fibonacci_sync(curr);
    }

    auto fib_1 = tpe->submit(fibonacci, tpe, curr - 1);
    auto fib_2 = tpe->submit(fibonacci, tpe, curr - 2);

    co_return co_await co_await fib_1 +
        co_await co_await fib_2;
}

int main() {
    concurrencpp::runtime runtime;
    auto fibb_30 = fibonacci(runtime.thread_pool_executor(), 30).get();
    std::cout << "fibonacci(30) = " << fibb_30 << std::endl;
    return 0;
}
```

### Result-promises

Result objects are the main way to pass data between tasks in concurrencpp and we've seen how executors and coroutines produce such objects.
Sometimes we want to use the capabilities of a result object with non-tasks, for example when using a third-party library. In this case, we can complete a result object by using a `result_promise`.
`result_promise` resembles a `std::promise` object - applications can manually set the asynchronous result or exception and make the associated `result` object become ready.

Just like result objects, result-promises are a move only type that becomes empty after move. Similarly, after setting a result or an exception, the result promise becomes empty as well.
If a result-promise gets out of scope and no result/exception has been set, the result-promise destructor sets a `concurrencpp::errors::broken_task` exception using the `set_exception` method.
Suspended and blocked tasks waiting for the associated result object are resumed/unblocked.

Result promises can convert callback style of code into `async/await` style of code: whenever a component requires a callback to marshal the asynchronous result, we can pass a callback that calls `set_result` or `set_exception` (depending on the asynchronous result itself) on the passed result promise, and return the associated result.

#### `result_promise` API

```cpp
template <class type>
class result_promise {    
    /*
        Constructs a valid result_promise.
    */
    result_promise();

    /*
        Moves the content of rhs to *this. After this call, rhs is empty.
    */        
    result_promise(result_promise&& rhs) noexcept;

    /*
        Destroys *this, possibly setting a concurrencpp::errors::broken_task exception
        by calling set_exception if *this is not empty at the time of destruction.
    */        
    ~result_promise() noexcept;

    /*
        Moves the content of rhs to *this. After this call, rhs is empty.
    */        
    result_promise& operator = (result_promise&& rhs) noexcept;

    /*
        Returns true if this is a non-empty result-promise.
        Applications must not use this object if this->operator bool() is false.
    */
    explicit operator bool() const noexcept;

    /*
        Sets a value by constructing <<type>> from arguments... in-place.
        Makes the associated result object become ready - tasks waiting for it to become ready are unblocked.
        Suspended tasks are resumed either inline or via the executor that was provided by calling result::await_via or result::resolve_via.
        After this call, *this becomes empty.
        If *this is empty, a concurrencpp::errors::empty_result_promise exception is thrown.
    */
    template<class ... argument_types>
    void set_result(argument_types&& ... arguments);
    
    /*
        Sets an exception.
        Makes the associated result object become ready - tasks waiting for it to become ready are unblocked.
        Suspended tasks are resumed either inline or via the executor that was provided by calling result::await_via or result::resolve_via.
        After this call, *this becomes empty.
        If *this is empty, a concurrencpp::errors::empty_result_promise exception is thrown.
        If exception_ptr is null, an std::invalid_argument exception is thrown.
    */
    void set_exception(std::exception_ptr exception_ptr);

    /*
        A convenience method that invokes a callable with arguments... and calls set_result with the result of the invocation.
        If an exception is thrown, the thrown exception is caught and set instead by calling set_exception.
        After this call, *this becomes empty.
        If *this is empty, a concurrencpp::errors::empty_result_promise exception is thrown.            
    */
    template<class callable_type, class ... argument_types>
    void set_from_function(callable_type&& callable, argument_types&& ... arguments);
    
    /*
        Gets the associated result object.
        If *this is empty, a concurrencpp::errors::empty_result_promise exception is thrown.
        If this method had been called before, a concurrencpp::errors::result_already_retrieved exception is thrown.
    */
    result<type> get_result();
};
```

#### *Example: Marshaling asynchronous result using* `result_promise`:
```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>

int main() {
    concurrencpp::result_promise<std::string> promise;
    auto result = promise.get_result();

    std::thread my_3_party_executor([promise = std::move(promise)] () mutable {
        std::this_thread::sleep_for(std::chrono::seconds(1)); //Imitate real work
        promise.set_result("hello world");
    });

    auto asynchronous_string = result.get();
    std::cout << "result promise returned string: " << asynchronous_string << std::endl;

    my_3_party_executor.join();
}
```
In this example, We use `std::thread` as a third-party executor. This represents a scenario when a non-concurrencpp executor is used as part of the application life-cycle. We extract the result object before we pass the promise and block the main thread until the result becomes ready. In `my_3_party_executor`, we set a result as if we `co_return`ed it.

### Shared result objects

Shared results are a special kind of result objects that allow multiple consumers to access the asynchronous result, similar to `std::shared_future`.  Different consumers from different threads can call functions like `await`, `get` and `resolve` in a thread safe manner.

Shared results are built from regular result objects and unlike regular result objects, they are both copyable and movable. As such, `shared_result` behaves like an `std::shared_ptr` object. If the shared result was moved to another instance, the shared result is empty, and trying to access it will throw an exception.

In order to support multiple consumers, the shared-result object will return a *reference* to asynchronous value instead of moving it (like a regular result object). For example, a `shared_result<int>`will return an `int&` when `get`,`await` etc. are called. If the underlying type of the `shared_result` is `void` or a reference type (like `int&`), they are returned as usual. If the asynchronous result is a thrown-exception, it is re-thrown.

Do note that while acquiring the asynchronous result using `shared_result` from multiple threads is thread-safe, the actual value might not be. For example, multiple threads can acquire an asynchronous integer by receiving its reference (`int&`). It *does not* make the integer itself thread safe. It is alright to mutate the asynchronous value if the asynchronous value is already thread safe. Alternatively, applications are encouraged to use `const` types to begin with (like `const int`), and acquire constant-references (like `const int&`) that prevent mutation.

#### `shared_result` API
```cpp
class share_result {
    /*
        Creates an empty shared-result that isn't associated with any task.
    */
    shared_result() noexcept = default;

    /*
        Destroys the shared-result. Associated tasks are not cancelled.
        The destructor does not block waiting for the asynchronous result to become ready.
    */    
    ~shared_result() noexcept = default;

    /*
        Converts a regular result object to a shared-result object.
        After this call, rhs is empty.
    */
    shared_result(result<type> rhs);

    /*
        Copy constructor. Creates a copy of the shared result object that monitors the same task.
    */
    shared_result(const shared_result&) noexcept = default;
        
    /*
        Move constructor. Moves rhs to *this. After this call, rhs is empty.
    */
    shared_result(shared_result&& rhs) noexcept = default;
        
    /*
        Copy assignment operator. Copies rhs to *this and monitors the same task that rhs monitors.  
    */        
    shared_result& operator=(const shared_result& rhs) noexcept;

    /*
        Move assignment operator. Moves rhs to *this. After this call, rhs is empty.
    */
    shared_result& operator=(shared_result&& rhs) noexcept;

    /*
        Returns true if this is a non-empty shared-result.
        Applications must not use this object if this->operator bool() is false.
    */
    explicit operator bool() const noexcept;

    /*
        Queries the status of *this.
        The return value is any of result_status::idle, result_status::value or result_status::exception.
        Throws concurrencpp::errors::empty_result if *this is empty.        
    */
    result_status status() const;

    /*
        Blocks the current thread of execution until this shared-result is ready, when status() != result_status::idle.
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    void wait();

    /*
        Blocks until this shared-result is ready or duration has passed. Returns the status of this shared-result after unblocking.
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    template<class duration_type, class ratio_type>
    result_status wait_for(std::chrono::duration<duration_type, ratio_type> duration);

    /*
        Blocks until this shared-result is ready or timeout_time has reached. Returns the status of this result after unblocking.
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    template<class clock_type, class duration_type>
    result_status wait_until(std::chrono::time_point<clock_type, duration_type> timeout_time);

    /*
        Blocks the current thread of execution until this shared-result is ready, when status() != result_status::idle.
        If the result is a valid value, a reference to it is returned, otherwise, get rethrows the asynchronous exception.        
        Throws concurrencpp::errors::empty_result if *this is empty.                    
    */
    std::add_lvalue_reference_t<type> get();

    /*
        Returns an awaitable used to await this shared-result.
        If the shared-result is already ready - the current coroutine resumes immediately in the calling thread of execution.
        If the shared-result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready,
        by the thread which had set the asynchronous value or exception.
        In either way, after resuming, if the result is a valid value, a reference to it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws concurrencpp::errors::empty_result if *this is empty.                            
    */
    auto operator co_await();

    
    /*
        Returns an awaitable used to await this shared-result.
        If the shared-result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready,
        by scheduling the current coroutine via executor.
        If the result is already ready - the behaviour depends on the value of force_rescheduling:
            If force_rescheduling = true, then the current coroutine is forcefully suspended and resumed via executor.
            If force_rescheduling = false, then the current coroutine resumes immediately in the calling thread of execution.
        In either way, after resuming, if the result is a valid value, a reference to it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws concurrencpp::errors::empty_result if *this is empty.        
        Throws std::invalid_argument if executor is null.
        If this shared-result is ready and force_rescheduling=true, throws any exception that executor::enqueue may throw.    
    */
    auto await_via(std::shared_ptr<concurrencpp::executor> executor, bool force_rescheduling = true);

    
    /*
        Returns an awaitable used to resolve this shared-result.
        After co_await expression finishes, *this is returned in a non-empty form, in a ready state.
        Throws concurrencpp::errors::empty_result if *this is empty.
    */    
    auto resolve();

    /*
        Returns an awaitable used to resolve this shared-result.
        If the shared-result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready,
        by scheduling the current coroutine via executor.
        If the result is already ready - the behaviour depends on the value of force_rescheduling:
            If force_rescheduling = true, then the current coroutine is forcefully suspended and resumed via executor.
            If force_rescheduling = false, then the current coroutine resumes immediately in the calling thread of execution.
        In either way, after resuming, *this is returned in a non-empty form and guaranteed that its status is not result_status::idle.
        Throws concurrencpp::errors::empty_result if *this is empty.        
        Throws std::invalid_argument if executor is null.
        If this shared-result is ready and force_rescheduling=true, throws any exception that executor::enqueue may throw.                    
    */
    auto resolve_via(std::shared_ptr<concurrencpp::executor> executor, bool force_rescheduling = true);
};
```

#### `shared_result` example
```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <chrono>

concurrencpp::result<void> consume_shared_result(concurrencpp::shared_result<int> shared_result,
    std::shared_ptr<concurrencpp::executor> resume_executor) {
    std::cout << "Awaiting shared_result to have a value" << std::endl;

    const auto& async_value = co_await shared_result.await_via(resume_executor);

    std::cout << "In thread id " << std::this_thread::get_id() << ", got: " << async_value << ", memory address: " << &async_value << std::endl;
}

int main() {
    concurrencpp::runtime runtime;
    auto result = runtime.background_executor()->submit([] {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 100;
    });

    concurrencpp::shared_result<int> shared_result(std::move(result));
    concurrencpp::result<void> results[8];

    for (size_t i = 0; i < 8; i++) {
        results[i] = consume_shared_result(shared_result, runtime.thread_pool_executor());
    }

    std::cout << "Main thread waiting for all consumers to finish" << std::endl;

    auto all_consumed = concurrencpp::when_all(std::begin(results), std::end(results));
    all_consumed.get();

    std::cout << "All consumers are done, exiting" << std::endl;
    return 0;
}
```


###  Summery: using tasks and coroutines

A task is an asynchronous operation implementing an asynchronous computational step. Tasks are created by using one of the executor methods or by invoking a concurrencpp coroutine. Tasks might return a result object that is used to consume the asynchronous value or exception the task had produced.  When used correctly, result objects don't block, this way we can chain tasks together, creating a bigger, asynchronous flow graph that never blocks.

A concurrencpp coroutine is a C++ suspendable function. It is eager, meaning it starts to run the moment it is invoked.
It returns one of `concurrencpp::result` / `concurrencpp::null_result` and contains any of `co_await` or `co_return` in its body. Parallel coroutines are a special kind of coroutines that start run in another thread, by passing a `concurrencpp::executor_tag` and an instance of a valid concurrencpp executor as the first arguments.

### Result auxiliary functions

For completeness, concurrencpp provides helper functions that help manipulate result objects:

```cpp
/*
    Creates a ready result object by building <<type>> from arguments&&... in-place.
*/
template<class type, class ... argument_types>
result<type> make_ready_result(argument_types&& ... arguments);

/*
    An overload for void type.
*/
result<void> make_ready_result();

/*
    Creates a ready result object from an exception pointer.
    The returned result object will re-throw exception_ptr when calling get, await or await_via.
    Throws std::invalid_argument if exception_ptr is null.
*/
template<class type>
result<type> make_exceptional_result(std::exception_ptr exception_ptr);

/*
    Overload. Similar to make_exceptional_result(std::exception_ptr),
    but gets an exception object directly.
*/
template<class type, class exception_type>
result<type> make_exceptional_result(exception_type exception);
 
/*
    Creates a result object that becomes ready when all the input results become ready.
    Passed result objects are emptied and returned as a tuple.
    Throws std::invalid_argument if any of the passed result objects is empty.
*/
template<class ... result_types>
result<std::tuple<typename std::decay<result_types>::type...>>
   when_all(result_types&& ... results);

/*
    Overload. Similar to when_all(result_types&& ...) but receives a pair of iterators referencing a range.
    Passed result objects are emptied and returned as a vector.
    If begin == end, the function returns immediately with an empty vector.
    Throws std::invalid_argument if any of the passed result objects is empty.
*/
template<class iterator_type>
result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>
   when_all(iterator_type begin, iterator_type end);

/*
    Overload. Returns a ready result object that doesn't monitor any asynchronous result.
*/
result<std::tuple<>> when_all();

/*
    Helper struct returned from when_any.
    index is the position of the ready result in results sequence.
    results is either an std::tuple or an std::vector of the results that were passed to when_any.
*/
template <class sequence_type>
struct when_any_result {
    std::size_t index;
    sequence_type results;
};

/*
    Creates a result object that becomes ready when at least one of the input results is ready.
    Passed result objects are emptied and returned as a tuple.
    Throws std::invalid_argument if any of the passed result objects is empty.
*/
template<class ... result_types>
result<when_any_result<std::tuple<result_types...>>>
   when_any(result_types&& ... results);

/*
    Overload. Similar to when_any(result_types&& ...) but receives a pair of iterators referencing a range.
    Passed result objects are emptied and returned as a vector.
    Throws std::invalid_argument if begin == end.
    Throws std::invalid_argument if any of the passed result objects is empty.
*/
template<class iterator_type>
result<when_any_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>>
   when_any(iterator_type begin, iterator_type end);
```

### Timers and Timer queues

concurrencpp also provides timers and timer queues.
Timers are objects that define asynchronous actions running on an executor within a well-defined interval of time.
There are three types of timers - *regular timers*, *onshot-timers* and *delay objects*.

Regular timers have four properties that define them:

1. Callable - a callable that will be scheduled to run as a task periodically.
2. Executor - an executor that schedules the callable to run periodically.
3. Due time - from the time of creation, the interval in milliseconds the timer will be scheduled to run for the first time.
4. Frequency - from the time the timer was scheduled to run for the first time, the interval in milliseconds the callable will be scheduled to run periodically, until the timer is destructed or cancelled.

Like other objects in concurrencpp, timers are a move only type that can be empty.
When a timer is destructed or `timer::cancel` is called, the timer cancels its scheduled but not yet executed tasks. Ongoing tasks are uneffected. The timer callable must be thread safe. It is recommended to set the due time and the frequency of timers to a granularity of 50 milliseconds. 

A timer queue is a concurrencpp worker that manages a collection of timers and processes them in just one thread of execution. It is also the agent used to create new timers.
When a timer deadline (whether it is the timer's due-time or frequency) has reached, the timer queue "fires" the timer by scheduling its callable to run on the associated executor as a task.

Just like executors, timer queues also adhere to the RAII concept. When the runtime object gets out of scope, It shuts down the timer queue, cancelling all pending timers. After a timer queue has been shut down, any subsequent call to `make_timer`, `make_onshot_timer` and `make_delay_object` will throw an `errors::runtime_shutdown` exception.
Applications must not try to shut down timer queues by themselves.

#### `timer_queue` API:
```cpp   
class timer_queue {
    /*
        Destroys this timer_queue.
    */
    ~timer_queue() noexcept;
    
    /*
        Shuts down this timer_queue:
        Tells the underlying thread of execution to quit and joins it.
        Cancels all pending timers.
        After this call, invocation of any method besides shutdown and shutdown_requested will throw an errors::runtime_shutdown.
        If shutdown had been called before, this method has no effect.
    */
    void shutdown() noexcept;

    /*
        Returns true if shutdown had been called before, false otherwise.
    */
    bool shutdown_requested() const noexcept;

    /*
        Creates a new running timer where *this is the associated timer_queue.
        Throws std::invalid_argument if executor is null.
        Throws errors::runtime_shutdown if shutdown had been called before.
    */
    template<class callable_type, class ... argumet_types>
    timer make_timer(
        std::chrono::milliseconds due_time,
        std::chrono::milliseconds frequency,
        std::shared_ptr<concurrencpp::executor> executor,
        callable_type&& callable,
        argumet_types&& ... arguments);

    /*
        Creates a new one-shot timer where *this is the associated timer_queue.
        Throws std::invalid_argument if executor is null.
        Throws errors::runtime_shutdown if shutdown had been called before.
    */
    template<class callable_type, class ... argumet_types>
    timer make_one_shot_timer(
        std::chrono::milliseconds due_time,
        std::shared_ptr<concurrencpp::executor> executor,
        callable_type&& callable,
        argumet_types&& ... arguments);

    /*
        Creates a new delay object where *this is the associated timer_queue.
        Throws std::invalid_argument if executor is null.
        Throws errors::runtime_shutdown if shutdown had been called before.
    */
    result<void> make_delay_object(
        std::chrono::milliseconds due_time,
        std::shared_ptr<concurrencpp::executor> executor);
};
```

#### `timer` API:

```cpp   
class timer {
    /*
        Creates an empty timer.
    */
    timer() noexcept = default;

    /*
        Cancels the timer, if not empty.
    */
    ~timer() noexcept;

    /*
        Moves the content of rhs to *this.
        rhs is empty after this call.
    */
    timer(timer&& rhs) noexcept = default;

    /*
        Moves the content of rhs to *this.
        rhs is empty after this call.
        Returns *this.
    */
    timer& operator = (timer&& rhs) noexcept;

    /*
        Cancels this timer.
        After this call, the associated timer_queue will not schedule *this to run again and *this becomes empty.
        Scheduled, but not yet executed tasks are cancelled.
        Ongoing tasks are uneffected.
        This method has no effect if *this is empty or the associated timer_queue has already expired.
    */
    void cancel();

    /*
        Returns the associated executor of this timer.    
        Throws concurrencpp::errors::empty_timer is *this is empty.
    */
    std::shared_ptr<executor> get_executor() const;

    /*
        Returns the associated timer_queue of this timer.
        Throws concurrencpp::errors::empty_timer is *this is empty.
    */
    std::weak_ptr<timer_queue> get_timer_queue() const;

    /*
        Returns the due time of this timer.
        Throws concurrencpp::errors::empty_timer is *this is empty.
    */
    std::chrono::milliseconds get_due_time() const;

    /*
        Returns the frequency of this timer.    
        Throws concurrencpp::errors::empty_timer is *this is empty.
    */
    std::chrono::milliseconds get_frequency() const;

    /*
        Sets new frequency for this timer.
        Callables already scheduled to run at the time of invocation are not affected.    
        Throws concurrencpp::errors::empty_timer is *this is empty.
    */
    void set_frequency(std::chrono::milliseconds new_frequency);

    /*
        Returns true is *this is not an empty timer, false otherwise.
        The timer should not be used if this->operator bool() is false.
    */
   explicit  operator bool() const noexcept;
};
```

#### *Regular timer example:*
```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>

using namespace std::chrono_literals;

int main() {
    concurrencpp::runtime runtime;
    std::atomic_size_t counter = 1;
    concurrencpp::timer timer = runtime.timer_queue()->make_timer(
        1500ms,
        2000ms,
        runtime.thread_pool_executor(),
        [&] {
            const auto c = counter.fetch_add(1);
            std::cout << "timer was invoked for the " << c << "th time" << std::endl;
        });

    std::this_thread::sleep_for(12s);
    return 0;
}
```
In this example we create a regular timer  by using the timer queue. The timer schedules its callable after 1.5 seconds, then fires its callable every 2 seconds. The given callable runs in the threadpool executor.

#### Oneshot timers

A oneshot timer is a one-time timer with only a due time - after it schedules its callable to run once it never reschedules it to run again.  

#### *Oneshot timer example:*
```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>

using namespace std::chrono_literals;

int main() {
    concurrencpp::runtime runtime;
    concurrencpp::timer timer = runtime.timer_queue()->make_one_shot_timer(
        3000ms,
        runtime.thread_executor(),
        [&] {
            std::cout << "hello and goodbye" << std::endl;
        });

    std::this_thread::sleep_for(4s);
    return 0;
}
```
In this example, we create a timer that runs only once - after 3 seconds from its creation, the timer will schedule to run its callable on a new thread of execution (using `concurrencpp::thread_executor`).

#### Delay objects

A delay object is a result object that becomes ready when its due time is reached. Applications can `co_await` this result object to delay the current coroutine in a non-blocking way.  The current coroutine is resumed by the executor that was passed to `make_delay_object`.

#### *Delay object example:*
```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>

using namespace std::chrono_literals;

concurrencpp::null_result delayed_task(
    std::shared_ptr<concurrencpp::timer_queue> tq,
    std::shared_ptr<concurrencpp::thread_pool_executor> ex) {
    size_t counter = 1;

    while(true) {
        std::cout << "task was invoked " << counter << " times." << std::endl;
        counter++;

        co_await tq->make_delay_object(1500ms, ex);
    }
}

int main() {
    concurrencpp::runtime runtime;
    delayed_task(runtime.timer_queue(), runtime.thread_pool_executor());

    std::this_thread::sleep_for(10s);
    return 0;
}
```

In this example, we created a coroutine (that does not marshal any result or thrown exception), which delays itself in a loop by calling `co_await` on a delay object.

### The runtime object
 
The concurrencpp runtime object is the agent used to acquire, store and create new executors.  
The runtime must be created as a value type as soon as the main function starts to run.
When the concurrencpp runtime gets out of scope, it iterates over its stored executors and shuts them down one by one by calling `executor::shutdown`. Executors then exit their inner work loop and any subsequent attempt to schedule a new task will throw a `concurrencpp::runtime_shutdown` exception. The runtime also contains the global timer queue used to create timers and delay objects.
Upon destruction, stored executors will destroy unexecuted tasks, and wait for ongoing tasks to finish. If an ongoing task tries to use an executor to spawn new tasks or schedule its own task continuation - an exception will be thrown. In this case, ongoing tasks need to quit as soon as possible, allowing their underlying executors to quit. The timer queue will also be shut down, cancelling all running timers.  With this RAII style of code, no tasks can be processed before the creation of the runtime object, and while/after the runtime gets out of scope.
This frees concurrent applications from needing to communicate termination messages explicitly. Tasks are free use executors as long as the runtime object is alive.

#### `runtime` API

```cpp
class runtime {
    /*
        Creates a runtime object with default options.    
    */
    runtime();

    /*
        Creates a runtime object with user defined options.
    */
    runtime(const concurrencpp::runtime_options& options);

    /*
        Destroys this runtime object.
        Calls executor::shutdown on each monitored executor.
        Calls timer_queue::shutdown on the global timer queue.
    */
    ~runtime() noexcept;

    /*
        Returns this runtime timer queue used to create new times.
    */
    std::shared_ptr<concurrencpp::timer_queue> timer_queue() const noexcept;

    /*
        Returns this runtime concurrencpp::inline_executor
    */
    std::shared_ptr<concurrencpp::inline_executor> inline_executor() const noexcept;

    /*
        Returns this runtime concurrencpp::thread_pool_executor
    */
    std::shared_ptr<concurrencpp::thread_pool_executor> thread_pool_executor() const noexcept;

    /*
        Returns this runtime concurrencpp::background_executor
    */
    std::shared_ptr<concurrencpp::thread_pool_executor> background_executor() const noexcept;

    /*
        Returns this runtime concurrencpp::thread_executor
    */
    std::shared_ptr<concurrencpp::thread_executor> thread_executor() const noexcept;

    /*
        Creates a new concurrencpp::worker_thread_executor and registers it in this runtime.
        Might throw std::bad_alloc or std::system_error if any underlying memory or system resource could not have been acquired.
    */
    std::shared_ptr<concurrencpp::worker_thread_executor> make_worker_thread_executor();

    /*
        Creates a new concurrencpp::manual_executor and registers it in this runtime.
        Might throw std::bad_alloc or std::system_error if any underlying memory or system resource could not have been acquired.
    */
    std::shared_ptr<concurrencpp::manual_executor> make_manual_executor();

    /*
        Creates a new user defined executor and registers it in this runtime.
        executor_type must be a valid concrete class of concurrencpp::executor.
        Might throw std::bad_alloc if no memory is available.
        Might throw any exception that the constructor of <<executor_type>> might throw.
    */
    template<class executor_type, class ... argument_types>
    std::shared_ptr<executor_type> make_executor(argument_types&& ... arguments);

    /*
        returns the version of concurrencpp that the library was built with.
    */
    static std::tuple<unsigned int, unsigned int, unsigned int> version() noexcept;
};
```

#### Creating user-defined executors

As mentioned before, Applications can create their own custom executor type by inheriting the `derivable_executor` class.
There are a few points to consider when implementing user defined executors:
The most important thing is to remember that executors are used from multiple threads, so implemented methods must be thread-safe.

New executors can be created using `runtime::make_executor`. Applications must not create new executors with plain instantiation (such as `std::make_shared` or plain `new`), only by using  `runtime::make_executor`. Also, applications must not try to re-instantiate the built-in concurrencpp executors, like the `thread_pool_executor` or the `thread_executor`, those executors must only be accessed through their existing instance in the runtime object.

Another important point is to handle shutdown correctly: `shutdown`, `shutdown_requested` and `enqueue` should all monitor the executor state and behave accordingly when invoked:
* `shutdown` should tell underlying threads to quit and then join them.
 * `shutdown` might be called multiple times, and the method must handle this scenario by ignoring any subsequent call to `shutdown` after the first invocation.
* `enqueue` must throw a `concurrencpp::errors::runtime_shutdown` exception if `shutdown` had been called before.

Implementing an executor is one of the rare cases applications need to work with `concurrencpp::task` class directly. `concurrencpp::task` is a `std::function` like object, but with a few differences.
Like `std::function`, the task object stores a callable that acts as the asynchronous operation.
Unlike `std::function`, `concurrencpp::task` is a move only type. On invocation, task objects receive no parameters and return `void`. Moreover, every task object can be invoked only once. After the first invocation, the task object becomes empty.
Invoking an empty task object is equivalent to invoking an empty lambda (`[]{}`), and will not throw any exception.
Task objects receive their callable as a forwarding reference (`type&&` where `type` is a template parameter), and not by copy (like `std::function`). Construction of the stored callable happens in-place. This allows task objects to contain callables that are move-only type (like `std::unique_ptr` and `concurrencpp::result`).
Task objects try to use different methods to optimize the usage of the stored types.
Task objects apply the short-buffer-optimization (sbo) for regular, small callables, and will inline calls to `std::coroutine_handle<void>` by calling them directly without virtual dispatch.    

#### `task` API

```cpp
  class task {
    /*
        Creates an empty task object.
    */
        task() noexcept;
        
    /*
        Creates a task object by moving the stored callable of rhs to *this.
            If rhs is empty, then *this will also be empty after construction.
            After this call, rhs is empty.
        */
        task(task&& rhs) noexcept;

    /*
        Creates a task object by storing callable in *this.
        <<typename std::decay<callable_type>::type>> will be in-place-
        constructed inside *this by perfect forwarding callable.
    */
        template<class callable_type>
        task(callable_type&& callable);

    /*
        Destroys stored callable, does nothing if empty.
    */
        ~task() noexcept;

        task(const task& rhs) = delete;
        task& operator=(const task&& rhs) = delete;

    /*
        If *this is empty, does nothing.
        Invokes stored callable, and immediately destroys it.
        After this call, *this is empty.
        May throw any exception that the invoked callable may throw.
    */
        void operator()();

    /*
        Moves the stored callable of rhs to *this.
        If rhs is empty, then *this will also be empty after this call.    
        If *this already contains a stored callable, operator = destroys it first.
    */
        task& operator=(task&& rhs) noexcept;

    /*
        If *this is not empty, task::clear destroys the stored callable and empties *this.
        If *this is empty, clear does nothing.
    */
        void clear() noexcept;

    /*
        Returns true if *this stores a callable. false otherwise.
    */
       expliit  operator bool() const noexcept;

    /*
        Returns true if *this stores a callable,
        and that stored callable has the same type as <<typename std::decay<callable_type>::type>>  
    */
        template<class callable_type>
        bool contains() const noexcept;
    };
```
When implementing user-defined executors, it is up to the implementation to store tasks (when `enqueue` is called), and execute them according to the executor inner-mechanism.

#### *Example: using a user-defined executor:*

```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class logging_executor : public concurrencpp::derivable_executor<logging_executor> {

private:
    mutable std::mutex _lock;
    std::queue<concurrencpp::task> _queue;
    std::condition_variable _condition;
    bool _shutdown_requested;
    std::thread _thread;
    const std::string _prefix;

    void work_loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(_lock);
            if (_shutdown_requested) {
                return;
            }

            if (!_queue.empty()) {
                auto task = std::move(_queue.front());
                _queue.pop();
                lock.unlock();
                std::cout << _prefix << " A task is being executed" << std::endl;
                task();
                continue;
            }

            _condition.wait(lock, [this] {
                return !_queue.empty() || _shutdown_requested;
            });
        }
    }

public:
    logging_executor(std::string_view prefix) :
        derivable_executor<logging_executor>("logging_executor"),
        _shutdown_requested(false),
        _prefix(prefix) {
        _thread = std::thread([this] {
            work_loop();
        });
    }

    void enqueue(concurrencpp::task task) override {
        std::cout << _prefix << " A task is being enqueued!" << std::endl;

        std::unique_lock<std::mutex> lock(_lock);
        if (_shutdown_requested) {
            throw concurrencpp::errors::runtime_shutdown("logging executor - executor was shutdown.");
        }

        _queue.emplace(std::move(task));
        _condition.notify_one();
    }

    void enqueue(std::span<concurrencpp::task> tasks) override {
        std::cout << _prefix << tasks.size() << " tasks are being enqueued!" << std::endl;

        std::unique_lock<std::mutex> lock(_lock);
        if (_shutdown_requested) {
            throw concurrencpp::errors::runtime_shutdown("logging executor - executor was shutdown.");
        }

        for (auto& task : tasks) {
            _queue.emplace(std::move(task));
        }

        _condition.notify_one();
    }

    int max_concurrency_level() const noexcept override {
        return 1;
    }

    bool shutdown_requested() const noexcept override {
        std::unique_lock<std::mutex> lock(_lock);
        return _shutdown_requested;
    }

    void shutdown() noexcept override {
        std::cout << _prefix << " shutdown requested" << std::endl;

        std::unique_lock<std::mutex> lock(_lock);
        if (_shutdown_requested) return; //nothing to do.
        _shutdown_requested = true;
        lock.unlock();

        _condition.notify_one();
        _thread.join();
    }
};

int main() {
    concurrencpp::runtime runtime;
    auto logging_ex = runtime.make_executor<logging_executor>("Session #1234");

    for (size_t i = 0; i < 10; i++) {
        logging_ex->post([] {
            std::cout << "hello world" << std::endl;
        });
    }

    std::getchar();
    return 0;
}
```

In this example, we created an executor which logs actions like enqueuing a task or executing it. We implement the `executor` interface, and we request the runtime to create and store an instance of it by calling `runtime::make_executor`. The rest of the application behaves exactly the same as if we were to use non user-defined executors.

### Supported platforms and tools

* **Operating systems:** Linux, macOS, Windows (Windows 10 and above)
* **Compilers:** MSVC (Visual Studio 2019 version 16.8.2 and above), Clang (Clang-11 and above)
* **Tools:** CMake (3.16 and above) 

### Building, installing and testing

##### Building the library on Windows (release mode)
```cmake
$ git clone https://github.com/David-Haim/concurrencpp.git
$ cd concurrencpp
$ cmake -S . -B build/lib
$ cmake --build build/lib --config Release
```
##### Running the tests on Windows (debug + release mode)
```cmake
$ git clone https://github.com/David-Haim/concurrencpp.git
$ cd concurrencpp
$ cmake -S test -B build/test
$ cmake --build build/test
    <# for release mode: cmake --build build/test --config Release #>
$ cd build/test
$ ctest . -V -C Debug
    <# for release mode: ctest . -V -C Release #>
```
##### Building the library on *nix platforms (release mode)
```cmake
$ git clone https://github.com/David-Haim/concurrencpp.git
$ cd concurrencpp
$ cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/lib
$ cmake --build build/lib
    #optional, install the library: sudo cmake --install build/lib
```
##### Running the tests on *nix platforms 

With clang, it is also possible to run the tests with TSAN (thread sanitizer) support.

```cmake
$ git clone https://github.com/David-Haim/concurrencpp.git
$ cd concurrencpp
$ cmake -S test -B build/test
  #for release mode: cmake -DCMAKE_BUILD_TYPE=Release -S test -B build/test
  #for TSAN mode: cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_THREAD_SANITIZER=Yes -S test -B build/test
$ cmake --build build/test  
$ cd build/test
$ ctest . -V
```
