## concurrencpp, the C++ concurrency library

(note: documentation is still under development)

concurrencpp allows applications to write asynchronous code easily and safetly by using executors and coroutines.
By using concurrencpp applications can break down big procedures that need to be processed asynchronously into smaller tasks that run concurrently and work in a co-operative manner to achieve the wanted result. 
concurrencpp also allows applications to write parallel algorithms more easily by using parallel coroutines.

concurrencpp main advantages are: 
* Being able to write non blocking, asynchronous code easily by using the C++20 coroutines and the `co_await` keyword.
* Being able to write modern concurrent code without having to rely on low-level concurrency primitives like locks and condition variables 
* The concurrency runtime manages all low-level resources such as threads automatically
* concurrencpp provides various types of commonly used executors with a complete coroutines integration. 
* Applications can extend the library by using their own provided executors.

**Executors**

In the context of concurrencpp, an executor is an object that is able to schedule and run coroutines.
Executors simplify the work of managing resources such as threads and task queues by decoupling them away from application code.
Executors provide a unified way of scheduling and executing coroutines, since they all extend `concurrencpp::executor`.
The main power of concurrencpp executors comes from their ability to schedule user provided callables by turning them into coroutines first,
scheduling them to run asynchronously, and finally marshal the asynchronous result (or exception) back to the caller.

*Executor types:*
As mentioned above, concurrencpp provides commonly used executors. These excutore types are:
* **threadpool executor** - a general purpose executor that maintains a pool of threads. 
This executor is suitable for relatively short cpu-bound tasks that don't block. Applications 
are encouraged to use this executor as the default executor for non blocking functions.  

* **blocking executor** - a thredpool executor with a larger pool of threads. Suitable for launching short blocking tasks 
like file io and db queries.

* **thread executor** - an executor that launches an enqueued task on a new thread of execution. Threads are not reused. 
This executor is good for long running actions, like objects that run a work loop, or long blocking actions.

* **worker thread executor** - a single thread executor that maintains a single task queue. Suitable when applications want 
a dedicated thread that executes many related tasks.

* **manual executor** - an executor that does not execute coroutines by itself. Application code can execute previously enqueued tasks 
by manually invoking the executor methods. 

* **inline executor** - mainly used to override the behavior of other executors. Enqueuing a task is equivalent to invoking it inline.
 
**Result objects**

Asynchronous values and exceptions can be consumed using the concurrencpp result objects.
A result object is a conduit for the asynchronous coroutine result. 
When a coroutine finishes execution, it either returns a valid value or throws an exception. 
In either case, this asynchronous result is marshaled to the consumer of the result object.
The result state therefore, very from `idle` (the asynchronous result or exception aren't ready yet) to `value`  (the coroutine terminated by returning a valid value) to `exception` (the coroutine terminated by throwing an exception).  
Result objects can be polled for their state, waited, resolved or awaited. 
Awaiting a result object by using `co_await` (and by doing so, turning the current function into a coroutine as well) is the preferred way of consuming result objects.  
concurrencpp also provide the `null_result` type. in this case, any returned value or thrown exception will be dropped. 

**Coroutines**

There are few ways to create and schedule coroutines in concurrencpp

-  By calling any of `executor::post`, `executor::submit`, `executor::bulk_post` or `executor::bulk_submit`. 
In this case the application provides a callable and depending on the selected method, might return a result object
to the caller. the callable is then turned into a coroutine and scheduled to run in the executor.

-  By using a parallel coroutine: 
In concurrencpp, a parallel coroutine is a function that 

1.  Returns any of `concurrencpp::result<T>` / `concurrencpp::null_result` 
1. Gets `concurrencpp::executor_tag` as its first argument 
1. Gets any of `type*` / `type&` / `std::shared_ptr<type>`, where `type` is a concrete class of `concurrencpp::executor` 
1. Contains any of `co_await` or `co_return` in its body.

In this case, the function is a parallel coroutine: concurrencpp will start the coroutine suspended and immediately schedule it to re-run in the provided executor. Applications can then consume the result of the parallel coroutine by using the returned result object. This technique is especially good for parallel algorithms that require recursion.

-  By retunring a result object.
When a function returns any of concurrencpp::result / concurrencpp::null_result and has at least on of co_await/co_return,
then this function is a coroutine that starts executing immediately in the caller thread without suspension. 

**Timers and Timer queues**

concurrencpp also provides timers and timer queues. 
Timers are objects that schedule actions to run on an executor within a well defined interval of time.
A timer queue is a concurrencpp worker that manages a collection of timers and processes them in just one thread of execution.

Timers have 4 different properties associated with them:

1.  A callable (and potentially its argument) - the callable that will be scheduled to run periodically.
1. An executor - in which the callable will be scheduled to run as a coroutine
1.  due time - from the time of creation, the interval in milliseconds in which the timer will be scheduled to run for the first time 
1.  frequency - from the time the timer's callable is scheduled for the first time, the interval in milliseconds the callable will be schedule to run periodically, until the timer is destructed or cancelled.

There are two more flavors of timers in concurrencpp: oneshot timers and a delay objects. 

*  A oneshot timer is a one-time timer with only a due time - after it schedules its callable to run once it never reschedule it to run again.  

* A delay object is a result object that becomes ready when its due time is reached. Applications can co_await on this result object 
for a non-blocking way to delay the execution of a coroutine. 

**The runtime object**

The concurrencpp runtime object is the glue that sticks all the components above to a complete and cohesive mechanism of managing asynchronous actions. 
The concurrencpp runtime object is the agent used to acquire, store and create new executors. The runtime must be created as a value type as soon as the main function starts to run. 
When the concurrencpp runtime gets out of scope, it iterates over its stored executors and shuts them down one by one by calling `executor::shutdown`. Executors then exit their inner work loop and any subsequent attempt to schedule a new coroutine will throw a `concurrencpp::executor_shutdown` exception. The runtime also contains the global timer queue used to create timers and delay objects.
