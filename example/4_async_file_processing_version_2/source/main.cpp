/*
        This example shows another version of processing a file asynchronously but this time
        we will use concurrencpp::resume_on to explicitly switch execution between executors.

        First we will reschedule the replace_chars_in_file task to run on the background executor in order to read
        the file. Then we will reschedule the task to run on the threadpool executor and replace the unwanted character.
        lastly we will reschedule the task to run again on the background executor in order to write the processed
        content back ot the file.

        The original version of this application does this rescheduling implicitly, by awaiting results that executor::submit
        produces. this version explicitly switches execution between executors and does not create sub-tasks from lambdas.
        Both versions are identical in terms of functionality and the final outcome.
*/

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "concurrencpp/concurrencpp.h"

concurrencpp::result<void> replace_chars_in_file(std::shared_ptr<concurrencpp::thread_pool_executor> background_executor,
                                                 std::shared_ptr<concurrencpp::thread_pool_executor> threadpool_executor,
                                                 const std::string file_path,
                                                 char from,
                                                 char to) {
    // switch control from whatever executor/thread this coroutine was called from to the background_executor.
    co_await concurrencpp::resume_on(background_executor);

    // we're running on the background executor now. we can safely block.
    std::vector<char> file_content;

    {
        std::ifstream input;
        input.exceptions(std::ifstream::badbit);
        input.open(file_path, std::ios::binary);
        file_content.assign(std::istreambuf_iterator<char>(input), {});
    }

    // switch execution to the threadpool-executor
    co_await concurrencpp::resume_on(threadpool_executor);

    // we're running on the threadpool executor now. we can process CPU-bound tasks now.
    for (auto& c : file_content) {
        if (c == from) {
            c = to;
        }
    }

    // switch execution back to the background-executor
    concurrencpp::resume_on(background_executor);

    // write the processed file content. since we're running on the background executor, it's safe to block.
    std::ofstream output;
    output.exceptions(std::ofstream::badbit);
    output.open(file_path, std::ios::binary);
    output.write(file_content.data(), file_content.size());

    std::cout << "file has been modified successfully" << std::endl;
}

int main(int argc, const char* argv[]) {
    if (argc < 4) {
        const auto help_msg = "please pass all necessary arguments\n\
argv[1] - the file to process\n\
argv[2] - the character to replace\n\
argv[3] - the character to replace with";

        std::cerr << help_msg << std::endl;
        return -1;
    }

    if (std::strlen(argv[2]) != 1 || std::strlen(argv[3]) != 1) {
        std::cerr << "argv[2] and argv[3] must be one character only" << std::endl;
        return -1;
    }

    const auto file_path = std::string(argv[1]);
    const auto from_char = argv[2][0];
    const auto to_char = argv[3][0];

    concurrencpp::runtime runtime;

    try {
        replace_chars_in_file(runtime.background_executor(), runtime.thread_pool_executor(), file_path, from_char, to_char).get();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
