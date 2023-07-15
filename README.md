# concurrencpp, the C++ concurrency library

![Latest Release](https://img.shields.io/github/v/release/David-Haim/concurrencpp.svg) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

concurrencpp brings the power of concurrent tasks to the C++ world, allowing developers to write highly concurrent applications easily and safely by using tasks, executors and coroutines.
By using concurrencpp applications can break down big procedures that need to be processed asynchronously into smaller tasks that run concurrently and work in a co-operative manner to achieve the wanted result.
concurrencpp also allows applications to write parallel algorithms easily by using parallel coroutines.

concurrencpp main advantages are:
* Writing modern concurrency code using higher level tasks instead of low level primitives like `std::thread` and `std::mutex`.
* Writing highly concurrent and parallel applications that scale automatically to use all hardware resources, as needed.
* Achieving non-blocking, synchronous-like code easily by using C++20 coroutines and the `co_await` keyword.
* Reducing the possibility of race conditions, data races and deadlocks by using high-level objects with built-in synchronization.
* concurrencpp provides various types of commonly used executors with a complete coroutine integration.
* Applications can extend the library by implementing their own provided executors.
* concurrencpp is mature and well tested on various platforms and operating systems.

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
	* [`result` type](#result-type)
    * [`result` API](#result-api)
	* [`lazy_result` type](#lazy_result-type)
    * [`lazy_result` API](#lazy_result-api)
* [Parallel coroutines](#parallel-coroutines)
    * [Parallel Fibonacci example](#parallel-fibonacci-example)
* [Result-promises](#result-promises)
    * [`result_promise` API](#result_promise-api)
    * [`result_promise` example](#result_promise-example)
* [Shared result objects](#shared-result-objects)
    * [`shared_result` API](#shared_result-api)
    * [`shared_result` example](#shared_result-example)
* [Termination in concurrencpp](#termination-in-concurrencpp)
* [Resume executors](#resume-executors)
* [Utility functions](#utility-functions)
    * [`make_ready_result`](#make_ready_result-function)
    * [`make_exceptional_result`](#make_exceptional_result-function)
    * [`when_all`](#when_all-function)
    * [`when_any`](#when_any-function)
    * [`resume_on`](#resume_on-function)
* [Timers and Timer queues](#timers-and-timer-queues)
    * [`timer_queue` API](#timer_queue-api)
    * [`timer` API](#timer-api)
    * [Regular timer example](#regular-timer-example)
    * [Oneshot timers](#oneshot-timers)
    * [Oneshot timer example](#oneshot-timer-example)
    * [Delay objects](#delay-objects)
    * [Delay object example](#delay-object-example)
* [Generators](#generators)     
	* [`generator` API](#generator-api)
	* [`generator` example](#generator-example)
* [Asynchronous locks](#asynchronous-locks)     
	* [`async_lock` API](#async_lock-api)
	* [`scoped_async_lock` API](#scoped_async_lock-api)
	* [`async_lock` example](#async_lock-example)
* [Asynchronous condition variable](#asynchronous-condition-variables)     
	* [`async_condition_variable` API](#async_condition_variable-api)
	* [`async_condition_variable` example](#async_condition_variable-example)
* [The runtime object](#the-runtime-object)
    * [`runtime` API](#runtime-api)
    * [Thread creation and termination monitoring](#thread-creation-and-termination-monitoring)
    * [Creating user-defined executors](#creating-user-defined-executors)
    * [`task` objects](#task-objects)
    * [`task` API](#task-api)
    * [Writing a user-defined executor example](#example-writing-a-user-defined-executor)
* [Supported platforms and tools](#supported-platforms-and-tools)
* [Building, installing and testing](#building-installing-and-testing)

----

###  concurrencpp overview

concurrencpp is built around the concept of concurrent tasks. A task is an asynchronous operation. Tasks offer a higher level of abstraction for concurrent code than traditional thread-centric approaches. Tasks can be chained together, meaning that tasks pass their asynchronous result from one to another, where the result of one task is used as if it were a parameter or an intermediate value of another ongoing task. Tasks allow applications to utilize available hardware resources better and scale much more than using raw threads, since tasks can be suspended, awaiting another task to produce a result, without blocking underlying OS-threads. Tasks bring much more productivity to developers by allowing them to focus more on business-logic and less on low-level concepts like thread management and inter-thread synchronization.

While tasks specify *what* actions have to be executed, *executors* are worker-objects that specify *where and how* to execute tasks. Executors spare applications the tedious management of thread pools and task queues. Executors also decouple those concepts away from application code, by providing a unified API for creating and scheduling tasks.

Tasks communicate with each other using *result objects*. A result object is an asynchronous pipe that pass the asynchronous result of one task to another ongoing-task. Results can be awaited and resolved in a non-blocking manner.

These three concepts - the task, the executor and the associated result are the building blocks of concurrencpp. Executors run tasks that communicate with each other by sending results through result-objects. Tasks, executors and result objects work together symbiotically to produce concurrent code which is fast and clean.

concurrencpp is built around the RAII concept. In order to use tasks and executors, applications create a `runtime` instance in the beginning of the `main` function. The runtime is then used to acquire existing executors and register new user-defined executors. Executors are used to create and schedule tasks to run, and they might return a `result` object that can be used to pass the asynchronous result to another task that acts as its consumer.
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

In this basic example, we created a runtime object, then we acquired the thread executor from the runtime. We used `submit` to pass a lambda as our given callable. This lambda returns `void`, hence, the executor returns a `result<void>` object that passes the asynchronous result back to the caller.  `main` calls  `get` which blocks the main thread until the result becomes ready. If no exception was thrown, `get` returns `void`. If an exception was thrown, `get` re-throws it. Asynchronously, `thread_executor` launches a new thread of execution and runs the given lambda. It implicitly `co_return void` and the task is finished. `main` is then unblocked.
      
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

concurrencpp allows applications to produce and consume coroutines as the main way of creating tasks. concurrencpp supports both eager and lazy tasks.

Eager tasks start to run the moment they are invoked. This type of execution is recommended when applications need to fire an asynchronous action and consume its result later on (fire and consume later), or completely ignore the asynchronous result (fire and forget).
 
Eager tasks can return  `result` or `null_result`. `result` return type tells the coroutine to pass the returned value or the thrown exception (fire and consume later)  while `null_result` return type tells the coroutine to drop and ignore any of them (fire and forget).

Eager coroutines can start to run synchronously, in the caller thread. This kind of coroutines is called "regular coroutines".
Concurrencpp eager coroutines can also start to run in parallel, inside a given executor, this kind of coroutines is called "parallel coroutines".

Lazy tasks, on the other hand, start to run only when `co_await`ed. This type of tasks is recommended when the result of the task is meant to be consumed immediately after creating the task. Lazy tasks, being deferred, are a bit more optimized for the case of immediate-consumption, as they do not need special thread-synchronization in order  to pass the asynchronous result back to its consumer. The compiler might also optimize away some memory allocations needed to form the underlying coroutine promise. It is not possible to fire a lazy task and execute something else meanwhile  - the firing of a lazy-callee coroutine necessarily means the suspension of the caller-coroutine. The caller coroutine will only be resumed when the lazy-callee coroutine completes. Lazy tasks can only return `lazy_result`.  

Lazy tasks can be converted to eager tasks by calling  `lazy_result::run`. This method runs the lazy task inline and returns a `result` object that monitors the newly started task. If developers are unsure which result type to use, they are encouraged to use lazy results, as they can be converted to regular (eager) results if needed.  

When a function returns any of `lazy_result`, `result` or `null_result`and contains at least one `co_await` or `co_return` in its body, the function is a concurrencpp coroutine. Every valid concurrencpp coroutine is a valid task. In our count-even example above, `count_even` is such a coroutine. We first spawned `count_even`, then inside it the threadpool executor spawned more child tasks (that are created from regular callables),  that were eventually joined using `co_await`.

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
        Turns a callable and its arguments into a task object and
        schedules it to run in this executor using enqueue.
        Arguments are passed to the task by decaying them first.
        Throws errors::runtime_shutdown exception if shutdown has been called before.
    */
    template<class callable_type, class ... argument_types>
    void post(callable_type&& callable, argument_types&& ... arguments);
    
    /*
        Like post, but returns a result object that passes the asynchronous result.
        Throws errors::runtime_shutdown exception if shutdown has been called before.
    */
    template<class callable_type, class ... argument_types>
    result<type> submit(callable_type&& callable, argument_types&& ... arguments);

    /*
        Turns an array of callables into an array of tasks and
        schedules them to run in this executor using enqueue.
        Throws errors::runtime_shutdown exception if shutdown has been called before.
    */
    template<class callable_type>
    void bulk_post(std::span<callable_type> callable_list);

    /*
        Like bulk_post, but returns an array of result objects that passes the asynchronous results.
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

* **background executor** - a threadpool executor with a larger pool of threads. Suitable for launching short blocking tasks like file io and db queries. Important note: when consuming results this executor returned by calling `submit` and `bulk_submit`, it is important to switch execution using `resume_on` to a cpu-bound executor, in order to prevent cpu-bound tasks to be processed inside background_executor.

example:
```cpp
    auto result = background_executor.submit([] { /* some blocking action */ });
    auto done_result = co_await result.resolve();
    co_await resume_on(some_cpu_executor);
    auto val = co_await done_result;  // runs inside some_cpu_executor
```
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
Applications can request executors to return a result object that passes the asynchronous result of the provided callable. This is done by calling `executor::submit` and `executor::bulk_submit`.
`submit` gets a callable, and returns a result object. `executor::bulk_submit` gets a `span` of callables and returns a `vector`of result objects in a similar way `submit` works.
In many cases, applications are not interested in the asynchronous value or exception. In this case, applications can use `executor:::post` and `executor::bulk_post` to schedule a callable or a `span` of callables to be executed, but also tells the task to drop any returned value or thrown exception. Not passing the asynchronous result is faster than passing , but then we have no way of knowing the status or the result of the ongoing task.

`post`, `bulk_post`, `submit` and `bulk_submit` use `enqueue` behind the scenes for the underlying scheduling mechanism.


#### `thread_pool_executor` API

Aside from `post`, `submit`, `bulk_post` and `bulk_submit`, the `thread_pool_executor`  provides these additional methods.  

```cpp
class thread_pool_executor {

    /*
        Returns the number of milliseconds each thread-pool worker
        remains idle (lacks any task to execute) before exiting.
        This constant can be set by passing a runtime_options object
        to the constructor of the runtime class.
    */
    std::chrono::milliseconds max_worker_idle_time() const noexcept;

};
```
#### `manual_executor` API

Aside from `post`, `submit`, `bulk_post` and `bulk_submit`, the `manual_executor`  provides these additional methods.

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
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    size_t size() const noexcept;
        
    /*
        Queries whether the executor is empty from tasks at the moment of invocation.
        This value can change quickly by the time the application handles it, it should be used as a hint.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    bool empty() const noexcept;

    /*
        Clears the executor from any enqueued but yet to-be-executed tasks,
        and returns the number of cleared tasks.
        Tasks enqueued to this executor by (post_)submit method are resumed
        and errors::broken_task exception is thrown inside them.
        Ongoing tasks that are being executed by loop_once(_XXX) or loop(_XXX) are uneffected.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    size_t clear();

    /*
        Tries to execute a single task. If at the moment of invocation the executor
        is empty, the method does nothing.
        Returns true if a task was executed, false otherwise.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws. 
        Throws errors::shutdown_exception if shutdown was called before.
    */
    bool loop_once();

    /*
        Tries to execute a single task.
        This method returns when either a task was executed or max_waiting_time
        (in milliseconds) has reached.
        If max_waiting_time is 0, the method is equivalent to loop_once.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    bool loop_once_for(std::chrono::milliseconds max_waiting_time);

    /*
        Tries to execute a single task.
        This method returns when either a task was executed or timeout_time has reached.
        If timeout_time has already expired, this method is equivalent to loop_once.
        If shutdown is called from another thread, this method
        returns and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    template<class clock_type, class duration_type>
    bool loop_once_until(std::chrono::time_point<clock_type, duration_type> timeout_time);
   
    /*
        Tries to execute max_count enqueued tasks and returns the number of tasks that were executed.
        This method does not wait: it returns when the executor
        becomes empty from tasks or max_count tasks have been executed.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    size_t loop(size_t max_count);

    /*
        Tries to execute max_count tasks.
        This method returns when either max_count tasks were executed or a
        total amount of max_waiting_time has passed.
        If max_waiting_time is 0, the method is equivalent to loop.
        Returns the actual amount of tasks that were executed.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    size_t loop_for(size_t max_count, std::chrono::milliseconds max_waiting_time);

    /*    
        Tries to execute max_count tasks.
        This method returns when either max_count tasks were executed or timeout_time has reached.
        If timeout_time has already expired, the method is equivalent to loop.
        Returns the actual amount of tasks that were executed.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    template<class clock_type, class duration_type>
    size_t loop_until(size_t max_count, std::chrono::time_point<clock_type, duration_type> timeout_time);
    
    /*
        Waits for at least one task to be available for execution.
        This method should be used as a hint,
        as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    void wait_for_task();

    /*
        This method returns when one or more tasks are available for
        execution or max_waiting_time has passed.    
        Returns true if at at least one task is available for execution, false otherwise.
        This method should be used as a hint, as other threads (calling loop, for example)
        might empty the executor, before this thread has a chance to do something
        with the newly enqueued tasks.
        If shutdown is called from another thread, this method
        returns and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    bool wait_for_task_for(std::chrono::milliseconds max_waiting_time);

    /*
        This method returns when one or more tasks are available for execution or timeout_time has reached.    
        Returns true if at at least one task is available for execution, false otherwise.
        This method should be used as a hint,
        as other threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method
        returns and throws errors::shutdown_exception.
        This method is thread safe.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    template<class clock_type, class duration_type>
    bool wait_for_task_until(std::chrono::time_point<clock_type, duration_type> timeout_time);
    
    /*
        This method returns when max_count or more tasks are available for execution.    
        This method should be used as a hint, as other threads
        (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe. 
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.
    */
    void wait_for_tasks(size_t max_count);

    /*
        This method returns when max_count or more tasks are available for execution
        or max_waiting_time (in milliseconds) has passed.    
        Returns the number of tasks available for execution when the method returns.
        This method should be used as a hint, as other
        threads (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe.  
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.    
    */
    size_t wait_for_tasks_for(size_t count, std::chrono::milliseconds max_waiting_time);

    /*
        This method returns when max_count or more tasks are available for execution
        or timeout_time is reached.    
        Returns the number of tasks available for execution when the method returns.
        This method should be used as a hint, as other threads
        (calling loop, for example) might empty the executor,
        before this thread has a chance to do something with the newly enqueued tasks.
        If shutdown is called from another thread, this method returns
        and throws errors::shutdown_exception.
        This method is thread safe.  
        Might throw std::system_error if one of the underlying synchronization primitives throws.
        Throws errors::shutdown_exception if shutdown was called before.    
    */
    template<class clock_type, class duration_type>
    size_t wait_for_tasks_until(size_t count, std::chrono::time_point<clock_type, duration_type> timeout_time);
        
};
```
### Result objects

Asynchronous values and exceptions can be consumed using concurrencpp result objects. The `result` type represents the asynchronous result of an eager task while `lazy_result` represents the deferred result of a lazy task. 

When a task (eager or lazy) completes, it either returns a valid value or throws an exception. In either case, this asynchronous result is passed to the consumer of the result object.

`result` objects form asymmetric coroutines - the execution of a caller-coroutine is not effected by the execution of a callee-coroutine, both coroutines can run independently. Only when consuming the result of the callee-coroutine, the caller-coroutine might be suspended awaiting the callee to complete. Up until that point both coroutines run independently. The callee-coroutine runs whether its result is consumed or not. 

`lazy_result` objects form symmetric coroutines - execution of a callee-coroutine happens only after the suspension of the caller-coroutine. When awaiting a lazy result, the current coroutine is suspended and the lazy task associated with the lazy result starts to run. After the callee-coroutine completes and yields a result, the caller-coroutine is resumed. If a lazy result is not consumed, its associated lazy task never starts to run. 

All result objects are a move-only type, and as such, they cannot be used after their content was moved to another result object. In this case, the result object is considered to be empty and attempts to call any method other than `operator bool` and `operator = ` will throw an exception.

After the asynchronous result has been pulled out of the result object (for example, by calling `get` or `operator co_await`), the result object becomes empty. Emptiness can be tested with `operator bool`.

Awaiting a result means to suspend the current coroutine until the result object is ready. If a valid value was returned from the associated task, it is returned from the result object. If the associated task throws an exception, it is re-thrown.
At the moment of awaiting, if the result is already ready, the current coroutine resumes immediately. Otherwise, it is resumed by the thread that sets the asynchronous result or exception.

Resolving a result is similar to awaiting it. The difference is that the `co_await` expression will return the result object itself,
in a non empty form, in a ready state. The asynchronous result can then be pulled by using `get` or `co_await`.

Every result object has a status indicating the state of the asynchronous result.
The result status varies from `result_status::idle` (the asynchronous result or exception haven't been produced yet) to `result_status::value` (the associated task terminated gracefully by returning a valid value) to `result_status::exception` (the task terminated by throwing an exception).  The status can be queried by calling  `(lazy_)result::status`. 

#### `result` type

The `result` type represents the result of an ongoing, asynchronous task, similar to `std::future`. 

Aside from awaiting and resolving result-objects, they can also be waited for by calling any of `result::wait`, `result::wait_for`, `result::wait_until` or `result::get`. Waiting for a result to finish is a blocking operation (in the case the asynchronous result is not ready), and will suspend the entire thread of execution waiting for the asynchronous result to become available. Waiting operations are generally discouraged and only allowed in root-level tasks or in contexts which allow it, like blocking the main thread waiting for the rest of the application to finish gracefully, or using `concurrencpp::blocking_executor` or `concurrencpp::thread_executor`.

Awaiting result objects by using `co_await` (and by doing so, turning the current function/task into a coroutine as well) is the preferred way of consuming result objects, as it does not block underlying threads.

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
        The returned value is any of result_status::idle, result_status::value or result_status::exception.
        Throws errors::empty_result if *this is empty.        
    */
    result_status status() const;

    /*
        Blocks the current thread of execution until this result is ready,
        when status() != result_status::idle.
        Throws errors::empty_result if *this is empty.
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if one of the underlying synchronization primitives throws.                    
    */
    void wait();

    /*
        Blocks until this result is ready or duration has passed. Returns the status
        of this result after unblocking.
        Throws errors::empty_result if *this is empty.  
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    template<class duration_unit, class ratio>
    result_status wait_for(std::chrono::duration<duration_unit, ratio> duration);

    /*
        Blocks until this result is ready or timeout_time has reached. Returns the status
        of this result after unblocking.
        Throws errors::empty_result if *this is empty.         
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    template< class clock, class duration >
    result_status wait_until(std::chrono::time_point<clock, duration> timeout_time);

    /*
        Blocks the current thread of execution until this result is ready,
        when status() != result_status::idle.
        If the result is a valid value, it is returned, otherwise, get rethrows the asynchronous exception.        
        Throws errors::empty_result if *this is empty.         
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if one of the underlying synchronization primitives throws.           
    */
    type get();

    /*
        Returns an awaitable used to await this result.
        If the result is already ready - the current coroutine resumes
        immediately in the calling thread of execution.
        If the result is not ready yet, the current coroutine is suspended
        and resumed when the asynchronous result is ready,
        by the thread which had set the asynchronous value or exception.
        In either way, after resuming, if the result is a valid value, it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws errors::empty_result if *this is empty.                            
    */
    auto operator co_await();

    /*
        Returns an awaitable used to resolve this result.
        After co_await expression finishes, *this is returned in a non-empty form, in a ready state.
        Throws errors::empty_result if *this is empty.
    */    
    auto resolve();
};
```
#### `lazy_result` type

A lazy result object represents the result of a deferred lazy task. 

`lazy_result` has the responsibility of both starting the associated lazy task and passing its deferred result back to its consumer. 
When awaited or resolved, the lazy result suspends the current coroutine and starts the associated lazy task. when the associated task completes, its asynchronous value is passed to the caller task, which is then resumed. 

Sometimes, an API might return a lazy result, but applications need its associated task to run eagerly (without suspending the caller task). In this case, lazy tasks can be converted to eager tasks by calling  `run` on its associated lazy result. In this case, the associated task will start to run inline, without suspending the caller task. The original lazy result is emptied and a valid `result` object that monitors the newly started task will be returned instead.

#### `lazy_result` API

```cpp
class lazy_result {
    /*
        Creates an empty lazy result that isn't associated with any task.
    */
    lazy_result() noexcept = default;

    /*
        Moves the content of rhs to *this. After this call, rhs is empty.
    */
    lazy_result(lazy_result&& rhs) noexcept;

    /*
	    Destroys the result. If not empty, the destructor destroys the associated task without resuming it.
    */
    ~lazy_result() noexcept;

    /*
        Moves the content of rhs to *this. After this call, rhs is empty. Returns *this.
        If *this is not empty, then operator= destroys the associated task without resuming it.
    */
    lazy_result& operator=(lazy_result&& rhs) noexcept;

    /*
        Returns true if this is a non-empty result.
        Applications must not use this object if this->operator bool() is false.
    */
    explicit operator bool() const noexcept;

    /*
        Queries the status of *this.
        The returned value is any of result_status::idle, result_status::value or result_status::exception.
        Throws errors::empty_result if *this is empty.  
    */
    result_status status() const;

    /*
        Returns an awaitable used to start the associated task and await this result.
        If the result is already ready - the current coroutine resumes immediately
        in the calling thread of execution.
        If the result is not ready yet, the current coroutine is suspended and
        resumed when the asynchronous result is ready,
        by the thread which had set the asynchronous value or exception.
        In either way, after resuming, if the result is a valid value, it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws errors::empty_result if *this is empty.   
    */
    auto operator co_await();

    /*
        Returns an awaitable used to start the associated task and resolve this result.
        If the result is already ready - the current coroutine resumes immediately
        in the calling thread of execution.
        If the result is not ready yet, the current coroutine is suspended and resumed
        when the asynchronous result is ready, by the thread which
        had set the asynchronous value or exception.
        After co_await expression finishes, *this is returned in a non-empty form, in a ready state.	
        Throws errors::empty_result if *this is empty.
    */
    auto resolve();

    /*
        Runs the associated task inline and returns a result object that monitors the newly started task.
        After this call, *this is empty. 
        Throws errors::empty_result if *this is empty.
        Might throw std::bad_alloc if fails to allocate memory.
    */
    result<type> run();
};
```

###  Parallel coroutines

Regular eager coroutines start to run synchronously in the calling thread of execution. Execution might shift to another thread of execution if a coroutine undergoes a rescheduling, for example by awaiting an unready result object inside it.
concurrencpp also provides parallel coroutines, which start to run inside a given executor, not in the invoking thread of execution. This style of scheduling coroutines is especially helpful when writing parallel algorithms, recursive algorithms and concurrent algorithms that use the fork-join model.

Every parallel coroutine must meet the following preconditions:

1. Returns any of `result` / `null_result` .
2. Gets `executor_tag` as its first argument .
3. Gets any of `type*` / `type&` / `std::shared_ptr<type>`, where `type` is a concrete class of `executor` as its second argument.
4. Contains any of `co_await` or `co_return` in its body.
5. Is not a member function or a lambda function

If all the above applies, the function is a parallel coroutine:
concurrencpp will start the coroutine suspended and immediately reschedule it to run in the provided executor.
`concurrencpp::executor_tag` is a dummy placeholder to tell the concurrencpp runtime that this function is not a regular function, it needs to start running inside the given executor.
If the executor passed to the parallel coroutine is null, the coroutine will not start to run and an `std::invalid_argument` exception will be thrown synchronously. 
If all preconditions are met, Applications can consume the result of the parallel coroutine by using the returned result object. 

#### Parallel Fibonacci example:

In this example, we calculate the 30-th member of the Fibonacci sequence in a parallel manner.
We start launching each Fibonacci step in its own parallel coroutine. The first argument is a dummy `executor_tag` and the second argument is the threadpool executor.
Every recursive step invokes a new parallel coroutine that runs in parallel. Each result is `co_return`ed to its parent task and acquired by using `co_await`.   
When we deem the input to be small enough to be calculated synchronously (when `curr <= 10`), we stop executing each recursive step in its own task and just solve the algorithm synchronously.

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
Sometimes we want to use the capabilities of result objects with non-tasks, for example when using a third-party library. In this case, we can complete a result object by using a `result_promise`.
`result_promise` resembles a `std::promise` object - applications can manually set the asynchronous result or exception and make the associated `result` object become ready.

Just like result objects, result-promises are a move only type that becomes empty after move. Similarly, after setting a result or an exception, the result promise becomes empty as well.
If a result-promise gets out of scope and no result/exception has been set, the result-promise destructor sets a `concurrencpp::errors::broken_task` exception using the `set_exception` method.
Suspended and blocked tasks waiting for the associated result object are resumed/unblocked.

Result promises can convert callback style of code into `async/await` style of code: whenever a component requires a callback to pass the asynchronous result, we can pass a callback that calls `set_result` or `set_exception` (depending on the asynchronous result itself) on the passed result promise, and return the associated result.

#### `result_promise` API

```cpp
template <class type>
class result_promise {    
    /*
        Constructs a valid result_promise.
        Might throw std::bad_alloc if fails to allocate memory.
    */
    result_promise();

    /*
        Moves the content of rhs to *this. After this call, rhs is empty.
    */        
    result_promise(result_promise&& rhs) noexcept;

    /*
        Destroys *this, possibly setting an errors::broken_task exception
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
        Makes the associated result object become ready - tasks waiting for it
        to become ready are unblocked.
        Suspended tasks are resumed inline.
        After this call, *this becomes empty.
        Throws errors::empty_result_promise exception If *this is empty.
        Might throw any exception that the constructor
        of type(std::forward<argument_types>(arguments)...) throws.
    */
    template<class ... argument_types>
    void set_result(argument_types&& ... arguments);
    
    /*
        Sets an exception.
        Makes the associated result object become ready - tasks waiting for it
        to become ready are unblocked.
        Suspended tasks are resumed inline.
        After this call, *this becomes empty.
        Throws errors::empty_result_promise exception If *this is empty.
        Throws std::invalid_argument exception if exception_ptr is null.
    */
    void set_exception(std::exception_ptr exception_ptr);

    /*
        A convenience method that invokes a callable with arguments... and calls set_result
        with the result of the invocation.
        If an exception is thrown, the thrown exception is caught and set instead by calling set_exception.
        After this call, *this becomes empty.
        Throws errors::empty_result_promise exception If *this is empty.
        Might throw any exception that callable(std::forward<argument_types>(arguments)...)
        or the contructor of type(type&&) throw. 
    */
    template<class callable_type, class ... argument_types>
    void set_from_function(callable_type&& callable, argument_types&& ... arguments);
    
    /*
        Gets the associated result object.
        Throws errors::empty_result_promise exception If *this is empty.
        Throws errors::result_already_retrieved exception if this method had been called before.
    */
    result<type> get_result();
};
```

#### `result_promise` example:

In this example, `result_promise` is used to push data from one thread, and it can be pulled from its associated `result` object from another thread. 

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

Shared results are built from regular result objects and unlike regular result objects, they are both copyable and movable. As such, `shared_result` behaves like `std::shared_ptr` type. If a shared result instance is moved to another instance, the instance becomes empty, and trying to access it will throw an exception.

In order to support multiple consumers, shared results return a *reference* to the asynchronous value instead of moving it (like a regular results). For example, a `shared_result<int>` returns an `int&` when `get`,`await` etc. are called. If the underlying type of the `shared_result` is `void` or a reference type (like `int&`), they are returned as usual. If the asynchronous result is a thrown-exception, it is re-thrown.

Do note that while acquiring the asynchronous result using `shared_result` from multiple threads is thread-safe, the actual value might not be thread safe. For example, multiple threads can acquire an asynchronous integer by receiving its reference (`int&`). It *does not* make the integer itself thread safe. It is alright to mutate the asynchronous value if the asynchronous value is already thread safe. Alternatively, applications are encouraged to use `const` types to begin with (like `const int`), and acquire constant-references (like `const int&`) that prevent mutation.

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
        Might throw std::bad_alloc if fails to allocate memory.
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
        Throws errors::empty_result if *this is empty.        
    */
    result_status status() const;

    /*
        Blocks the current thread of execution until this shared-result is ready,
        when status() != result_status::idle.
        Throws errors::empty_result if *this is empty.  
        Might throw std::system_error if one of the underlying synchronization primitives throws.                   
    */
    void wait();

    /*
        Blocks until this shared-result is ready or duration has passed.
        Returns the status of this shared-result after unblocking.
        Throws errors::empty_result if *this is empty.                    
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    template<class duration_type, class ratio_type>
    result_status wait_for(std::chrono::duration<duration_type, ratio_type> duration);

    /*
        Blocks until this shared-result is ready or timeout_time has reached.
        Returns the status of this result after unblocking.
        Throws errors::empty_result if *this is empty.  
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    template<class clock_type, class duration_type>
    result_status wait_until(std::chrono::time_point<clock_type, duration_type> timeout_time);

    /*
        Blocks the current thread of execution until this shared-result is ready,
        when status() != result_status::idle.
        If the result is a valid value, a reference to it is returned,
        otherwise, get rethrows the asynchronous exception.        
        Throws errors::empty_result if *this is empty.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
    */
    std::add_lvalue_reference_t<type> get();

    /*
        Returns an awaitable used to await this shared-result.
        If the shared-result is already ready - the current coroutine resumes
        immediately in the calling thread of execution.
        If the shared-result is not ready yet, the current coroutine is
        suspended and resumed when the asynchronous result is ready,
        by the thread which had set the asynchronous value or exception.
        In either way, after resuming, if the result is a valid value, a reference to it is returned.
        Otherwise, operator co_await rethrows the asynchronous exception.
        Throws errors::empty_result if *this is empty.                            
    */
    auto operator co_await();
  
    /*
        Returns an awaitable used to resolve this shared-result.
        After co_await expression finishes, *this is returned in a non-empty form, in a ready state.
        Throws errors::empty_result if *this is empty.
    */    
    auto resolve();
};
```

#### `shared_result` example:

In this example, a `result` object is converted to a `shared_result` object and a reference to an asynchronous `int` result is acquired by many tasks spawned with `thread_executor`.

```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <chrono>

concurrencpp::result<void> consume_shared_result(concurrencpp::shared_result<int> shared_result,
    std::shared_ptr<concurrencpp::executor> resume_executor) {
    std::cout << "Awaiting shared_result to have a value" << std::endl;

    const auto& async_value = co_await shared_result;
    concurrencpp::resume_on(resume_executor);

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

    auto tpe = runtime.thread_pool_executor();
    auto all_consumed = concurrencpp::when_all(tpe, std::begin(results), std::end(results)).run();
    all_consumed.get();

    std::cout << "All consumers are done, exiting" << std::endl;
    return 0;
}
```

### Termination in concurrencpp
When the runtime object gets out of scope of `main`, it iterates each stored executor and calls its `shutdown` method. Trying to access the timer-queue or any executor will throw an `errors::runtime_shutdown` exception. When an executor shuts down, it clears its inner task queues, destroying un-executed `task` objects. If a task object stores a concurrencpp-coroutine, that coroutine is resumed inline and an `errors::broken_task` exception is thrown inside it. 
In any case where  a `runtime_shutdown` or a `broken_task` exception is thrown, applications should terminate their current code-flow gracefully as soon as possible. Those exceptions should not be ignored.
Both `runtime_shutdown` and `broken_task` inherit from `errors::interrupted_task` base class, and this type can also be used in a `catch` clause to handle termination in a unified way.

### Resume executors
Many concurrencpp asynchronous actions require an instance of an executor as their *resume executor*. When an asynchronous action (implemented as a coroutine) can finish synchronously, it resumes immediately in the calling thread of execution. If the asynchronous action can't finish synchronously, it will be resumed when it finishes, inside the given resume-executor. 
For example, `when_any` utility function requires an instance of a resume-executor as its first argument. `when_any` returns a `lazy_result` which becomes ready when at least one given result becomes ready. If one of the results is already ready at the moment of calling `when_any`, the calling coroutine is resumed synchronously in the calling thread of execution. If not, the calling coroutine will be resumed when at least of result is finished, inside the given resume-executor. 
Resume executors are important because they mandate where coroutines are resumed in cases where it's not clear where a coroutine is supposed to be resumed (for example, in the case of `when_any` and `when_all`), or in cases where the asynchronous action is processed inside one of the concurrencpp workers, which are only used to process that specific action, and not application code.  

### Utility functions

#### `make_ready_result` function
`make_ready_result` creates a ready result object from given arguments. Awaiting such result will cause the current coroutine to resume immediately.  `get` and `operator co_await` will return the constructed value. 

```cpp
/*
    Creates a ready result object by building <<type>> from arguments&&... in-place.
    Might throw any exception that the constructor
    of type(std::forward<argument_types>(arguments)...) throws.
    Might throw std::bad_alloc exception if fails to allocate memory.
*/
template<class type, class ... argument_types>
result<type> make_ready_result(argument_types&& ... arguments);

/*
    An overload for void type.
    Might throw std::bad_alloc exception if fails to allocate memory.
*/
result<void> make_ready_result();
```
#### `make_exceptional_result` function
`make_exceptional_result` creates a ready result object from a given exception. Awaiting such result will cause the current coroutine to resume immediately.  `get` and `operator co_await` will re-throw the given exception.
```cpp
/*
    Creates a ready result object from an exception pointer.
    The returned result object will re-throw exception_ptr when calling get or await.
    Throws std::invalid_argument if exception_ptr is null.
    Might throw std::bad_alloc exception if fails to allocate memory.
*/
template<class type>
result<type> make_exceptional_result(std::exception_ptr exception_ptr);

/*
    Overload. Similar to make_exceptional_result(std::exception_ptr),
    but gets an exception object directly.
    Might throw any exception that the constructor of exception_type(std::move(exception)) might throw. 
    Might throw std::bad_alloc exception if fails to allocate memory.
*/
template<class type, class exception_type>
result<type> make_exceptional_result(exception_type exception);
```
#### `when_all` function

`when_all` is a utility function that creates a lazy result object which becomes ready when all input results are completed. Awaiting this lazy result returns all input-result objects in a ready state, ready to be consumed.

`when_all` function comes with three flavors - one that accepts a heterogeneous range of result objects, another that gets a pair of iterators to a range of result objects of the same type, and lastly an overload that accepts no results objects at all.  In the case of no input result objects  - the function returns a ready result object of an empty tuple.

If one of the passed result-objects is empty, an exception will be thrown. In this case, input-result objects are unaffected by the function and can be used again after the exception was handled. If all input result objects are valid, they are emptied by this function, and returned in a valid and ready state as the output result.   
Currently, `when_all` only accepts `result` objects.

All overloads accept a resume executor as their first parameter. When awaiting a result returned by `when_all`, the caller coroutine will be resumed by the given resume executor.  

```cpp 
/*
    Creates a result object that becomes ready when all the input results become ready.
    Passed result objects are emptied and returned as a tuple.
    Throws std::invalid_argument if any of the passed result objects is empty.
    Might throw an std::bad_alloc exception if no memory is available.
*/
template<class ... result_types>
lazy_result<std::tuple<typename std::decay<result_types>::type...>>
   when_all(std::shared_ptr<executor_type> resume_executor,
              result_types&& ... results);

/*
    Overload. Similar to when_all(result_types&& ...) but receives a pair of iterators referencing a range.
    Passed result objects are emptied and returned as a vector.
    If begin == end, the function returns immediately with an empty vector.
    Throws std::invalid_argument if any of the passed result objects is empty.
    Might throw an std::bad_alloc exception if no memory is available.
*/
template<class iterator_type>
lazy_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>
   when_all(std::shared_ptr<executor_type> resume_executor,
               iterator_type begin, iterator_type end);

/*
    Overload. Returns a ready result object that doesn't monitor any asynchronous result.
    Might throw an std::bad_alloc exception if no memory is available.
*/
lazy_result<std::tuple<>> when_all(std::shared_ptr<executor_type> resume_executor);
```
#### `when_any` function

`when_any` is a utility function that creates a lazy result object which becomes ready when at least one input result is completed. Awaiting this result will return a helper struct containing all input-result objects plus the index of the completed task. It could be that by the time of consuming the ready result, other results might have already completed asynchronously. Applications can call `when_any` repeatedly in order to consume ready results as they complete until all results are consumed.
 
`when_any` function comes with only two flavors - one that accepts a heterogeneous range of result objects and another that gets a pair of iterators to a range of result-objects of the same type. Unlike `when_all`, there is no meaning in awaiting at least one task to finish when the range of results is completely empty. Hence, there is no overload with no arguments. Also, the overload of two iterators will throw an exception if those iterators reference an empty range (when `begin == end`).   

If one of the passed result-objects is empty, an exception will be thrown. In any case an exception is thrown, input-result objects are unaffected by the function and can be used again after the exception was handled. If all input result objects are valid, they are emptied by this function, and returned in a valid state as the output result.  
Currently, `when_any` only accepts `result` objects. 

All overloads accept a resume executor as their first parameter. When awaiting a result returned by `when_any`, the caller coroutine will be resumed by the given resume executor.  

```cpp
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
    Might throw an std::bad_alloc exception if no memory is available.
*/
template<class ... result_types>
lazy_result<when_any_result<std::tuple<result_types...>>>
   when_any(std::shared_ptr<executor_type> resume_executor,
              result_types&& ... results);

/*
    Overload. Similar to when_any(result_types&& ...) but receives a pair of iterators referencing a range.
    Passed result objects are emptied and returned as a vector.
    Throws std::invalid_argument if begin == end.
    Throws std::invalid_argument if any of the passed result objects is empty.
    Might throw an std::bad_alloc exception if no memory is available.
*/
template<class iterator_type>
lazy_result<when_any_result<std::vector<typename std::iterator_traits<iterator_type>::value_type>>>
   when_any(std::shared_ptr<executor_type> resume_executor,
              iterator_type begin, iterator_type end);
```

#### `resume_on` function
`resume_on` returns an awaitable that suspends the current coroutine and resumes it inside given `executor`. This is an important function that makes sure a coroutine is running in the right executor. For example, applications might schedule a background task using the `background_executor` and await the returned result object. In this case, the awaiting coroutine will be resumed inside the background executor. A call to `resume_on` with another cpu-bound executor makes sure that cpu-bound lines of code will not run on the background executor once the background task is completed. 
If a task is re-scheduled to run on another executor using `resume_on`, but that executor is shut down before it can resume the suspended task, that task is resumed immediately and an `erros::broken_task` exception is thrown. In this case, applications need to quite gracefully.  
```cpp
/*
    Returns an awaitable that suspends the current coroutine and resumes it inside executor.
    Might throw any exception that executor_type::enqueue throws.
*/
template<class executor_type>
auto resume_on(std::shared_ptr<executor_type> executor);
```

### Timers and Timer queues

concurrencpp also provides timers and timer queues.
Timers are objects that define asynchronous actions running on an executor within a well-defined interval of time.
There are three types of timers - *regular timers*, *onshot-timers* and *delay objects*.

Regular timers have four properties that define them:

1. Callable - a callable that will be scheduled to run as a task periodically.
2. Executor - an executor that schedules the callable to run periodically.
3. Due time - from the time of creation, the interval in milliseconds in which the callable will be scheduled to run for the first time.
4. Frequency - from the time the callable is scheduled to run for the first time, the interval in milliseconds the callable will be scheduled to run periodically, until the timer is destructed or cancelled.

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
        After this call, invocation of any method besides shutdown
        and shutdown_requested will throw an errors::runtime_shutdown.
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
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if the one of the underlying synchronization primitives throws.
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
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if the one of the underlying synchronization primitives throws.
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
        Might throw std::bad_alloc if fails to allocate memory.
        Might throw std::system_error if the one of the underlying synchronization primitives throws.
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
        After this call, the associated timer_queue will not schedule *this
        to run again and *this becomes empty.
        Scheduled, but not yet executed tasks are cancelled.
        Ongoing tasks are uneffected.
        This method has no effect if *this is empty or the associated timer_queue has already expired.
        Might throw std::system_error if one of the underlying synchronization primitives throws.
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
   explicit operator bool() const noexcept;
};
```

#### Regular timer example:

In this example we create a regular timer by using the timer queue. The timer schedules its callable to run after 1.5 seconds, then fires its callable every 2 seconds. The given callable runs on the threadpool executor.

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

#### Oneshot timers

A oneshot timer is a one-time timer with only a due time - after it schedules its callable to run once it never reschedules it to run again.  

#### Oneshot timer example:

In this example, we create a timer that runs only once - after 3 seconds from its creation, the timer will schedule its callable to run on a new thread of execution (using `thread_executor`).

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

#### Delay objects

A delay object is a lazy result object that becomes ready when it's `co_await`ed and its due time is reached. Applications can `co_await` this result object to delay the current coroutine in a non-blocking way.  The current coroutine is resumed by the executor that was passed to `make_delay_object`.

#### Delay object example:

In this example, we spawn a task (that does not return any result or thrown exception), which delays itself in a loop by calling `co_await` on a delay object.

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

### Generators 
A generator is a lazy, synchronous coroutine that is able to produce a stream of values to consume. Generators use the `co_yield` keyword to yield values back to their consumers.

Generators are meant to be used synchronously - they can only use the `co_yield` keyword and **must not** use the `co_await` keyword. A generator will continue  to produce values as long as the `co_yield` keyword is called. 
If the `co_return` keyword is called (explicitly or implicitly), then the generator will stop producing values.  Similarly, if an exception is thrown then the generator will stop producing values and the thrown exception will be re-thrown to the consumer of the generator.

Generators are meant to be used in a `range-for` loop: Generators implicitly produce two iterators - `begin` and `end` which control the execution of the `for` loop. These iterators should not be handled or accessed manually.

When a generator is created, it starts as a lazy task. When its `begin` method is called, the generator is resumed for the first time and an iterator is returned. The lazy task is resumed repeatedly by calling `operator++` on the returned iterator. The returned iterator will be equal to `end` iterator when the generator finishes execution either by exiting gracefully or throwing an exception.  As mentioned earlier, this happens behind the scenes by the inner mechanism of the loop and the generator, and should not be called directly.

Like other objects in concurrencpp, Generators are a move-only type. After a generator was moved, it is considered empty and trying to access its inner methods (other than `operator bool`) will throw an exception. The emptiness of a generator should not generally occur - it is advised to consume generators upon their creation in a `for` loop and not to try to call their methods individually. 

#### `generator` API
```cpp
class generator {
    /*
        Move constructor. After this call, rhs is empty.
    */
    generator(generator&& rhs) noexcept;

    /*
        Destructor. Invalidates existing iterators.
    */
    ~generator() noexcept;

    generator(const generator& rhs) = delete;
    generator& operator=(generator&& rhs) = delete;
    generator& operator=(const generator& rhs) = delete;
    
    /*
        Returns true if this generator is not empty.
        Applications must not use this object if this->operator bool() is false.
    */
    explicit operator bool() const noexcept;

    /*
        Starts running this generator and returns an iterator.
        Throws errors::empty_generator if *this is empty.
        Re-throws any exception that is thrown inside the generator code.
    */
    iterator begin();

    /*
        Returns an end iterator.
    */
    static generator_end_iterator end() noexcept;
};

class generator_iterator {
  
    using value_type = std::remove_reference_t<type>;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    /*
        Resumes the suspended generator and returns *this.
        Re-throws any exception that was thrown inside the generator code.
    */
    generator_iterator& operator++();

    /*
        Post-increment version of operator++. 
    */
    void operator++(int);

    /*
        Returns the latest value produced by the associated generator.
    */
    reference operator*() const noexcept;
	  
    /*
        Returns a pointer to the latest value produced by the associated generator. 
    */
    pointer operator->() const noexcept;

    /*
        Comparision operators. 
    */
    friend bool operator==(const generator_iterator& it0, const generator_iterator& it1) noexcept;
    friend bool operator==(const generator_iterator& it, generator_end_iterator) noexcept;
    friend bool operator==(generator_end_iterator end_it, const generator_iterator& it) noexcept;
    friend bool operator!=(const generator_iterator& it, generator_end_iterator end_it) noexcept;
    friend bool operator!=(generator_end_iterator end_it, const generator_iterator& it) noexcept;
};
```    
#### `generator` example: 

In this example, we will write a generator that yields the n-th member of the Sequence `S(n) = 1 + 2 + 3 + ... + n`  where `n <= 100`:

```cpp
concurrencpp::generator<int> sequence() {
    int i = 1;
    int sum = 0;
    while (i <= 100) {
        sum += i;
        ++i;
        co_yield sum;
    }
}

int main() {
    for (auto value : sequence()) {
        std::cout << value << std::end;
    }
    return 0;
} 
```

### Asynchronous locks
Regular synchronous locks cannot be used safely inside tasks for a number of reasons:

 - Synchronous locks, such as `std::mutex`, are expected to be locked and unlocked in the same thread of execution. Unlocking a synchronous lock in a thread which had not locked it is undefined behavior. Since tasks can be suspended and resumed in any thread of execution, synchronous locks will break when used inside tasks.
 - Synchronous locks were created to work with *threads* and not with *coroutines*. If a synchronous lock is already locked by one thread, then when another thread tries to lock it, the entire thread of execution will be blocked and will be unblocked when the lock is released. This mechanism works well for traditional multi-threading paradigms but not for coroutines: with coroutines, we want *tasks* to be *suspended and resumed* without blocking or interfering with the execution of underlying threads and executors.    

  `concurrencpp::async_lock` solves those issues by providing a similar API to `std::mutex`, with the main difference that calls to `concurrencpp::async_lock` will return a lazy-result that can be `co_awaited` safely inside tasks.  If one task tries to lock an async-lock and fails, the task will be suspended, and will be resumed when the lock is unlocked and acquired by the suspended task. This allows executors to process a huge amount of tasks waiting to acquire a lock without expensive context-switching and expensive kernel calls. 

Similar to how `std::mutex` works, only one task can acquire `async_lock` at any given time, and a *read barrier* is place at the moment of acquiring. Releasing an async lock places a *write barrier* and allows the next task to acquire it, creating a chain of one-modifier at a time which sees the changes other modifiers had done and posts its modifications for the next modifiers to see.    

Like `std::mutex`, `concurrencpp::async_lock` ***is not recursive***. Extra attention must be given when acquiring such lock - A lock must not be acquired again in a task that has been spawned by another task which had already acquired the lock. In such case, an unavoidable dead-lock will occur.  Unlike other objects in concurrencpp, `async_lock` is neither copiable nor movable. 

Like standard locks, `concurrencpp::async_lock` is meant to be used with scoped wrappers which leverage C++ RAII idiom to ensure locks are always unlocked upon  function return or thrown exception. `async_lock::lock` returns a lazy-result of a scoped wrapper that calls `async_lock::unlock` on destruction. Raw uses of `async_lock::unlock` are discouraged. `concurrencpp::scoped_async_lock` acts as the scoped wrapper and provides an API which is almost identical to `std::unique_lock`. `concurrencpp::scoped_async_lock` is movable, but not copiable.

`async_lock::lock` and `scoped_async_lock::lock` require a resume-executor as their parameter. Upon calling those methods, if the lock is available for locking, then it is locked and the current task is resumed immediately. If not, then the current task is suspended, and will be resumed inside the given resume-executor when the lock is finally acquired. 

`concurrencpp::scoped_async_lock` wraps an `async_lock` and ensure it's properly unlocked. like `std::unique_lock`, there are cases it does not wrap any lock, and in this case it's considered to be empty.  An empty  `scoped_async_lock` can happen when it's defaultly constructed, moved, or `scoped_async_lock::release` method is called. An empty scoped-async-lock will not unlock any lock on destruction. 

Even if the scoped-async-lock is not empty, it does not mean that it owns the underlying async-lock and it will unlock it on destruction. Non-empty and non-owning scoped-async locks can happen if `scoped_async_lock::unlock` was called or the scoped-async-lock was constructed using `scoped_async_lock(async_lock&, std::defer_lock_t)` constructor.

#### `async_lock` API
```cpp
class async_lock {
    /*
        Constructs an async lock object.
    */
    async_lock() noexcept;
	
    /*
        Destructs an async lock object.
        *this is not automatically unlocked at the moment of destruction. 
    */	
    ~async_lock() noexcept;
	
    /*
        Asynchronously acquires the async lock. 
        If *this has already been locked by another non-parent task, the current task will be suspended
        and will be resumed when *this is acquired, inside resume_executor.
        If *this has not been locked by another task, then *this will be acquired and the current task will be resumed 
        immediately in the calling thread of execution.
        If *this has already been locked by a parent task, then unavoidable dead-lock will occur.
        Throws std::invalid_argument if resume_executor is null.
        Throws std::system error if one of the underlying synhchronization primitives throws.	
    */
    lazy_result<scoped_async_lock> lock(std::shared_ptr<executor> resume_executor);
       
    /*
        Tries to acquire *this in the calling thread of execution.
        Returns true if *this is acquired, false otherwise.
        In any case, the current task is resumed immediately in the calling thread of execution.
        Throws std::system error if one of the underlying synhchronization primitives throws.
    */
    lazy_result<bool> try_lock();
       
    /*
        Releases *this and allows other tasks (including suspended tasks waiting for *this) to acquire it.
        Throws std::system error if *this is not locked at the moment of calling this method.
        Throws std::system error if one of the underlying synhchronization primitives throws.	
    */
    void unlock();
};
```
#### `scoped_async_lock` API

```cpp
class scoped_async_lock {
    /*
        Constructs an async lock wrapper that does not wrap any async lock.
    */
    scoped_async_lock() noexcept = default;
	
    /*
        If *this wraps async_lock, this method releases the wrapped lock.
    */
    ~scoped_async_lock() noexcept;

    /*
        Moves rhs to *this.
        After this call, *rhs does not wrap any async lock.
    */
    scoped_async_lock(scoped_async_lock&& rhs) noexcept;

    /*
        Wrapps unlocked lock.
        lock must not be in acquired mode when calling this method.
    */
    scoped_async_lock(async_lock& lock, std::defer_lock_t) noexcept;
		
    /*
        Wrapps locked lock.
        lock must be already acquired when calling this method.
    */
    scoped_async_lock(async_lock& lock, std::adopt_lock_t) noexcept;

    /*
        Calls async_lock::lock on the wrapped locked, using resume_executor as a parameter.
        Throws std::invalid_argument if resume_executor is nulll.
        Throws std::system_error if *this does not wrap any lock.
        Throws std::system_error if wrapped lock is already locked.
        Throws any exception async_lock::lock throws.
    */
    lazy_result<void> lock(std::shared_ptr<executor> resume_executor);
	
    /*
        Calls async_lock::try_lock on the wrapped lock.
        Throws std::system_error if *this does not wrap any lock.
        Throws std::system_error if wrapped lock is already locked.
        Throws any exception async_lock::try_lock throws.
    */
    lazy_result<bool> try_lock();
	
    /*
        Calls async_lock::unlock on the wrapped lock.
        If *this does not wrap any lock, this method does nothing.
        Throws std::system_error if *this wraps a lock and it is not locked.
    */
    void unlock();

    /*
        Checks whether *this wraps a locked mutex or not.
        Returns true if wrapped locked is in acquired state, false otherwise.
    */
    bool owns_lock() const noexcept;

    /*
        Equivalent to owns_lock.
    */
    explicit operator bool() const noexcept;

    /*
        Swaps the contents of *this and rhs.
    */
    void swap(scoped_async_lock& rhs) noexcept;
	
    /*
        Empties *this and returns a pointer to the previously wrapped lock.
        After a call to this method, *this doesn't wrap any lock.			
        The previously wrapped lock is not released, 
        it must be released by either unlocking it manually through the returned pointer or by 
        capturing the pointer with another scoped_async_lock which will take ownerwhip over it.
    */
    async_lock* release() noexcept;
	
    /*
        Returns a pointer to the wrapped async_lock, or a null pointer if there is no wrapped async_lock. 
    */
    async_lock* mutex() const noexcept;
};
```
#### `async_lock` example:

In this example we push 10,000,000 integers to an `std::vector` object from different tasks concurrently, while using `async_lock` to make sure no data race occurs and the correctness of the internal state of that vector object is preserved.   

```cpp
#include "concurrencpp/concurrencpp.h"

#include <vector>
#include <iostream>

std::vector<size_t> numbers;
concurrencpp::async_lock lock;

concurrencpp::result<void> add_numbers(concurrencpp::executor_tag,
                                       std::shared_ptr<concurrencpp::executor> executor,
                                       size_t begin,
                                       size_t end) {
    for (auto i = begin; i < end; i++) {
        concurrencpp::scoped_async_lock raii_wrapper = co_await lock.lock(executor);
        numbers.push_back(i);
    }
}

int main() {
    concurrencpp::runtime runtime;
    constexpr size_t range = 10'000'000;
    constexpr size_t sections = 4;
    concurrencpp::result<void> results[sections];

    for (size_t i = 0; i < 4; i++) {
        const auto range_start = i * range / sections;
        const auto range_end = (i + 1) * range / sections;

        results[i] = add_numbers({}, runtime.thread_pool_executor(), range_start, range_end);
    }

    for (auto& result : results) {
        result.get();
    }

    std::cout << "vector size is " << numbers.size() << std::endl;

    // make sure the vector state has not been corrupted by unprotected concurrent accesses
    std::sort(numbers.begin(), numbers.end());
    for (size_t i = 0; i < range; i++) {
        if (numbers[i] != i) {
            std::cerr << "vector state is corrupted." << std::endl;
            return -1;
        }
    }

    std::cout << "succeeded pushing range [0 - 10,000,000] concurrently to the vector!" << std::endl;
    return 0;
}
```

### Asynchronous condition variables

`async_condition_variable` imitates the standard `condition_variable` and can be used safely with tasks alongside `async_lock`. `async_condition_variable` works with `async_lock` to suspend a task until some shared memory (protected by the lock) has changed. Tasks that want to monitor shared memory changes will lock an instance of `async_lock`, and call `async_condition_variable::await`.  This will atomically unlock the lock and suspend the current task until some modifier task notifies the condition variable. A modifier task acquires the lock, modifies the shared memory, unlocks the lock and call either `notify_one` or `notify_all`.
When a suspended task is resumed (using the resume executor that was given to `await`), it locks the lock again, allowing the task to continue from the point of suspension seamlessly.
Like `async_lock`, `async_condition_variable` is neither movable or copiable - it is meant to be created in one place and accessed by multiple tasks.

`async_condition_variable::await` overloads require a resume-executor, which will be used to resume the task, and a locked `scoped_async_lock`. `async_condition_variable::await` comes with two overloads - one that accepts a predicate and one that doesn't. The overload which does not accept a predicate will suspend the calling task immediately upon invocation until it's resumed by a call to `notify_*`. The overload which does accept a predicate works by letting the predicate inspect the shared memory and suspend the task repeatedly until the shared memory has reached its wanted state. schematically it works like calling 

```cpp
while (!pred()) { // pred() inspects the shared memory and returns true or false
	co_await await(resume_executor, lock); // suspend the current task until another task calls `notify_xxx`
}
```
Just like the standard condition variable, applications are encouraged to use the predicate-overload, as it allows more fine-grained control over suspensions and resumptions.
`async_condition_variable` can be used to write concurrent collections and data-structures like concurrent queues and channels.

Internally, `async_condition_variable` holds a suspension-queue, in which tasks enqueue themselves when they await the condition variable to be notified. When any of `notify_*` methods are called, the notifying task dequeues either one task or all of the tasks, depending on the invoked method. Tasks are dequeued from the suspension-queue in a fifo manner. 
For example, if Task A calls `await` and then Task B calls `await`, then Task C calls `notify_one`, then internally task A will be dequeued and and resumed. Task B will remain suspended until another call to `notify_one` or `notify_all` is called. If task A and task B are suspended and task C calls `notify_all`, then both tasks will be dequeued and resumed. 

#### `async_condition_variable` API
```cpp
class async_condition_variable {
	/*
		Constructor.
	*/
	async_condition_variable() noexcept;

	/*
		Atomically releases lock and suspends the current task by adding it to *this suspension-queue.
		Throws std::invalid_argument if resume_executor is null.
		Throws std::invalid_argument if lock is not locked at the moment of calling this method.
		Might throw std::system_error if the underlying std::mutex throws. 
	*/
	lazy_result<void> await(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock);

	/*
		Equivalent to:		
		while (!pred()) {
            co_await await(resume_executor, lock);
        }
		
		Might throw any exception that await(resume_executor, lock) might throw.
		Might throw any exception that pred might throw.
	*/
	template<class predicate_type>
	lazy_result<void> await(std::shared_ptr<executor> resume_executor, scoped_async_lock& lock, predicate_type pred);
	
	/*
		Dequeues one task from *this suspension-queue and resumes it, if any available at the moment of calling this method.
		The suspended task is resumed by scheduling it to run on the executor given when await was called.
		Might throw std::system_error if the underlying std::mutex throws. 
	*/
	void notify_one();
	
	/*
		Dequeues all tasks from *this suspension-queue and resumes them, if any available at the moment of calling this method.
		The suspended tasks are resumed by scheduling them to run on the executors given when await was called.
		Might throw std::system_error if the underlying std::mutex throws. 
	*/
	void notify_all();
};
```

#### `async_condition_variable` example:

In this example, `async_lock` and `async_condition_variable` work together to implement a concurrent queue that can be used to send data (in this example, integers) between tasks. Note that some methods return a `result` while another return `lazy_result`, showing how both eager and lazy tasks can work together.

```cpp
#include "concurrencpp/concurrencpp.h"

#include <queue>
#include <iostream>

using namespace concurrencpp;

class concurrent_queue {

   private:
    async_lock _lock;
    async_condition_variable _cv;
    std::queue<int> _queue;
    bool _abort = false;

   public:
    concurrent_queue() = default;

    result<void> shutdown(std::shared_ptr<executor> resume_executor) {
        {
            auto guard = co_await _lock.lock(resume_executor);
            _abort = true;
        }

        _cv.notify_all();
    }

    lazy_result<void> push(std::shared_ptr<executor> resume_executor, int i) {
        {
            auto guard = co_await _lock.lock(resume_executor);
            _queue.push(i);
        }

        _cv.notify_one();
    }

    lazy_result<int> pop(std::shared_ptr<executor> resume_executor) {
        auto guard = co_await _lock.lock(resume_executor); 
        co_await _cv.await(resume_executor, guard, [this] {
            return _abort || !_queue.empty();
        });

        if (!_queue.empty()) {
            auto result = _queue.front();
            _queue.pop();

            co_return result;        
        }

        assert(_abort);
        throw std::runtime_error("queue has been shut down.");
    }
};

result<void> producer_loop(executor_tag,
                           std::shared_ptr<thread_pool_executor> tpe,
                           concurrent_queue& queue,
                           int range_start,
                           int range_end) {
    for (; range_start < range_end; ++range_start) {
        co_await queue.push(tpe, range_start);
    }
}

result<void> consumer_loop(executor_tag, std::shared_ptr<thread_pool_executor> tpe, concurrent_queue& queue) {
    try {
        while (true) {
            std::cout << co_await queue.pop(tpe) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main() {
    runtime runtime;
    const auto thread_pool_executor = runtime.thread_pool_executor();
    concurrent_queue queue;

    result<void> producers[4];
    result<void> consumers[4];

    for (int i = 0; i < 4; i++) {
        producers[i] = producer_loop({}, thread_pool_executor, queue, i * 5, (i + 1) * 5);
    }

    for (int i = 0; i < 4; i++) {
        consumers[i] = consumer_loop({}, thread_pool_executor, queue);
    }

    for (int i = 0; i < 4; i++) {
        producers[i].get();
    }

    queue.shutdown(thread_pool_executor).get();

    for (int i = 0; i < 4; i++) {
        consumers[i].get();
    }

    return 0;
}
```


### The runtime object
 
The concurrencpp runtime object is the agent used to acquire, store and create new executors.  
The runtime must be created as a value type as soon as the main function starts to run.
When the concurrencpp runtime gets out of scope, it iterates over its stored executors and shuts them down one by one by calling `executor::shutdown`. Executors then exit their inner work loop and any subsequent attempt to schedule a new task will throw a `concurrencpp::runtime_shutdown` exception. The runtime also contains the global timer queue used to create timers and delay objects.
Upon destruction, stored executors destroy unexecuted tasks, and wait for ongoing tasks to finish. If an ongoing task tries to use an executor to spawn new tasks or schedule its own task continuation - an exception will be thrown. In this case, ongoing tasks need to quit as soon as possible, allowing their underlying executors to quit. The timer queue will also be shut down, cancelling all running timers.  With this RAII style of code, no tasks can be processed before the creation of the runtime object, and while/after the runtime gets out of scope.
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
#### Thread creation and termination monitoring

In some cases, applications are interested in monitoring thread creation and termination, for example, some memory allocators require new threads to be registered and unregistered upon their creation and termination.  The concurrencpp runtime allows setting a thread creation callback and a thread termination callback. those callbacks will be called whenever one of the concurrencpp workers create a new thread and when that thread is terminating. Those callbacks are always called from inside the created/terminating thread, so `std::this_thread::get_id` will always return the relevant thread ID.  The signature of those callbacks is `void callback (std::string_view thread_name)`. `thread_name` is a concurrencpp specific title that is given to the thread and can be observed in some debuggers that present the thread name. The thread name is not guaranteed to be unique and should be used for logging and debugging. 

In order to set a thread-creation callback and/or a thread termination callback, applications can set the `thread_started_callback` and/or `thread_terminated_callback` members of the `runtime_options` which is passed to the runtime constructor. Since those callbacks are copied to each concurrencpp worker that might create threads, those callbacks have to be copiable.    

#### Example: monitoring thread creation and termination

```cpp
#include "concurrencpp/concurrencpp.h"

#include <iostream>

int main() {
    concurrencpp::runtime_options options;
    options.thread_started_callback = [](std::string_view thread_name) {
        std::cout << "A new thread is starting to run, name: " << thread_name << ", thread id: " << std::this_thread::get_id()
                  << std::endl;
    };

    options.thread_terminated_callback = [](std::string_view thread_name) {
        std::cout << "A thread is terminating, name: " << thread_name << ", thread id: " << std::this_thread::get_id() << std::endl;
    };

    concurrencpp::runtime runtime(options);
    const auto timer_queue = runtime.timer_queue();
    const auto thread_pool_executor = runtime.thread_pool_executor();

    concurrencpp::timer timer =
        timer_queue->make_timer(std::chrono::milliseconds(100), std::chrono::milliseconds(500), thread_pool_executor, [] {
            std::cout << "A timer callable is executing" << std::endl;
        });

    std::this_thread::sleep_for(std::chrono::seconds(3));

    return 0;
}
```
Possible output:

```
A new thread is starting to run, name: concurrencpp::timer_queue worker, thread id: 7496
A new thread is starting to run, name: concurrencpp::thread_pool_executor worker, thread id: 21620
A timer callable is executing
A timer callable is executing
A timer callable is executing
A timer callable is executing
A timer callable is executing
A timer callable is executing
A thread is terminating, name: concurrencpp::timer_queue worker, thread id: 7496
A thread is terminating, name: concurrencpp::thread_pool_executor worker, thread id: 21620
```

#### Creating user-defined executors

Applications can create their own custom executor type by inheriting the `derivable_executor` class.
There are a few points to consider when implementing user defined executors:
The most important thing is to remember that executors are used from multiple threads, so implemented methods must be thread-safe.

New executors can be created using `runtime::make_executor`. Applications must not create new executors with plain instantiation (such as `std::make_shared` or plain `new`), only by using  `runtime::make_executor`. Also, applications must not try to re-instantiate the built-in concurrencpp executors, like the `thread_pool_executor` or the `thread_executor`, those executors must only be accessed through their existing instances in the runtime object.

Another important point is to handle shutdown correctly: `shutdown`, `shutdown_requested` and `enqueue` should all monitor the executor state and behave accordingly when invoked:
* `shutdown` should tell underlying threads to quit and then join them.
* `shutdown` might be called multiple times, and the method must handle this scenario by ignoring any subsequent calls to `shutdown` after the first invocation.
* `enqueue` must throw a `concurrencpp::errors::runtime_shutdown` exception if `shutdown` had been called before.

#### `task` objects
 
Implementing executors is one of the rare cases where applications need to work with `concurrencpp::task` class directly. `concurrencpp::task` is an `std::function` like object, but with a few differences.
Like `std::function`, the task object stores a callable that acts as the asynchronous operation.
Unlike `std::function`, `task` is a move only type. On invocation, task objects receive no parameters and return `void`. Moreover, every task object can be invoked only once. After the first invocation, the task object becomes empty.
Invoking an empty task object is equivalent to invoking an empty lambda (`[]{}`), and will not throw any exception.
Task objects receive their callable as a forwarding reference (`type&&` where `type` is a template parameter), and not by copy (like `std::function`). Construction of the stored callable happens in-place. This allows task objects to contain callables that are move-only type (like `std::unique_ptr` and `concurrencpp::result`).
Task objects try to use different methods to optimize the usage of the stored types, for example, task objects apply the short-buffer-optimization (sbo) for regular, small callables, and will inline calls to `std::coroutine_handle<void>` by calling them directly without virtual dispatch.    

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
    explicit operator bool() const noexcept;

    /*
        Returns true if *this stores a callable,
        and that stored callable has the same type as <<typename std::decay<callable_type>::type>>  
    */
    template<class callable_type>
    bool contains() const noexcept;

};
```
When implementing user-defined executors, it is up to the implementation to store `task` objects (when `enqueue` is called), and execute them according to the executor inner-mechanism.

#### Example: writing a user-defined executor:

In this example, we create an executor which logs actions like enqueuing tasks or executing them. We implement the `executor` interface, and we request the runtime to create and store an instance of it by calling `runtime::make_executor`. The rest of the application behaves exactly the same as if we were to use non user-defined executors.

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

### Supported platforms and tools

* **Operating systems:** Linux, macOS, Windows (Windows 10 and above)
* **Compilers:** MSVC (Visual Studio 2019 version 16.8.2 and above), Clang 14+, Clang 11-13 with libc++, GCC 13+
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

With clang and gcc, it is also possible to run the tests with TSAN (thread sanitizer) support.

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
##### Important note regarding Linux and libc++
When compiling on Linux, the library tries to use `libstdc++` by default. If you intend to use `libc++` as your standard library implementation, `CMAKE_TOOLCHAIN_FILE` flag should be specified as below: 

```cmake
$ cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/libc++.cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/lib
```

##### Installing concurrencpp with vcpkg or Conan

Alternatively to building and installing the library manually, developers may get stable releases of concurrencpp via the [vcpkg](https://vcpkg.io/) and [Conan](https://conan.io/) package managers:

vcpkg:
```shell
$ vcpkg install concurrencpp
```

Conan: [concurrencpp on ConanCenter](https://conan.io/center/concurrencpp)

##### Experimenting with the built-in sandbox
concurrencpp comes with a built-in sandbox program which developers can modify and experiment, without having to install or link the compiled library to a different code-base. In order to play with the sandbox, developers can modify `sandbox/main.cpp` and compile the application using the following commands:

##### Building and running the sandbox on Windows:
```cmake
$ cmake -S sandbox -B build/sandbox
$ cmake --build build/sandbox
    <# for release mode: cmake --build build/sandbox --config Release #>
$ ./build/sandbox <# runs the sandbox>
```
##### Building and running the sandbox on *nix platforms:
```cmake
$ cmake -S sandbox -B build/sandbox
  #for release mode: cmake -DCMAKE_BUILD_TYPE=Release -S sandbox -B build/sandbox
$ cmake --build build/sandbox  
$ ./build/sandbox #runs the sandbox
```