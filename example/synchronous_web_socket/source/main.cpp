/*
        A company develops a desktop application for weather updates and
   reports. The software uses a synchronous web-socket library to communicate
   with the weather server.

        Since connecting and reading from a websocket may block for a long time
   (up to hours and days, if no update is available), and in an un-predictable
   manner, neither the background_executor or a concurrencpp::timer is suitable
   for the task.

        We will use the concurrencpp::thread_executor to create a dedicated
   thread that runs a work-loop of receiving weather reports from the server and
   showing them to the end-user. this way, this long-running, blocking task can
        be executed asynchronously without blocking or interfering other tasks
   or threads.

        There are merits in using concurrencpp::thread_executor over a raw
   std:thread : 1) std::thread is a hard-coded executor. since all concurrencpp
   executors are polymorphic, executing tasks is decoupled away. if we want to
   change std::thread to let's say, a threadpool, we will have to rewrite these
   lines of code dealing with thread creation, task scheduling etc. with
   concurrencpp::thread_executor, we just have to change the underlying type of
   the executor, and let the dynamic dispatching do the work for us. 2)
   std::thread has to be manually joined or detached. every std::thread_executor
   is stored and monitored inside the concurrencpp runtime, and is joined
   automatically when the runtime object is destroyed.

        Do note that this example uses a mock websocket library and no real
   connection/reading is going to be executed. The constants are made up and
   results are hard-coded. This example is only good for showing how to use a
   potentially long running/blocking tasks with a dedicated thread.

        Library's API:

        mock_web_socket::web_socket {
                void open(std::string_view server_url); //blocking
                std::string receive_msg(); //blocking
        }

*/

#include "concurrencpp/concurrencpp.h"
#include "mock_web_socket.h"

#include <iostream>

class weather_reporter {

   private:
    std::atomic_bool m_cancelled;
    const std::string m_endpoint_url;
    mock_web_socket::web_socket m_web_socket;

    void work_loop() {
        std::cout << "weather_reporter::work_loop started running on thread " << std::this_thread::get_id() << std::endl;

        try {
            m_web_socket.open(m_endpoint_url);
        } catch (const std::exception& e) {
            std::cerr << "can't connect to the server " << e.what() << std::endl;
            return;
        }

        std::cout << "websocket connected successfully to the weather server, waiting for updates." << std::endl;

        while (!m_cancelled.load()) {
            const auto msg = m_web_socket.receive_msg();
            std::cout << msg << std::endl;
        }
    }

   public:
    weather_reporter(std::string_view endpoint_url) : m_cancelled(false), m_endpoint_url(endpoint_url) {}

    ~weather_reporter() noexcept {
        m_cancelled.store(true);
    }

    void launch() {
        work_loop();
    }
};

int main() {
    concurrencpp::runtime runtime;
    auto reporter = std::make_shared<weather_reporter>("wss://www.example.com/weather-server");
    runtime.thread_executor()->post([reporter]() mutable {
        reporter->launch();
    });

    std::cout << "weather reporter is on and running asynchronously. press any key to exit." << std::endl;

    // do other things in the main thread or in other threads without having the
    // websocket blocking on reads.

    std::getchar();
    return 0;
}
