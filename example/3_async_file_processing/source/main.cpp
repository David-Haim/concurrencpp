/*
   In this example, we will use concurrencpp executors to process a file asynchronously.
   This application will iterate the file characters (we assume
   ASCII) and replace a character with another. The application gets three
   parameters through the command line arguments:
   argv[0] - the path to the binary that created this process (not used in this example)
   argv[1] - a file path
   argv[2] - the character to replace
   argv[3] - the character to replace with.

   Since standard file streams are blocking, we would like to execute
   file-io operations using the background_executor, which its job is to execute
   relatively short-blocking tasks (like file-io).

   Processing the file content is a cpu-bound task (iterating over a binary
   buffer and potentially changing characters), so after reading the file we
   will resume execution in the thread_pool_executor,

   After the content has been modified, it is ready to be re-written back to
   the file. we will again schedule a blocking write operation to the
   background_executor.
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

    // tell the background executor to read a whole file to a buffer and return it
    auto file_content = co_await background_executor->submit([file_path] {
        std::ifstream input;
        input.exceptions(std::ifstream::badbit);
        input.open(file_path, std::ios::binary);
        std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});
        return buffer;
    });

    // tell the threadpool executor to process the file
    auto processed_file_content = co_await threadpool_executor->submit([file_content = std::move(file_content), from, to]() mutable {
        for (auto& c : file_content) {
            if (c == from) {
                c = to;
            }
        }

        return std::move(file_content);
    });

    // schedule the write operation on the background executor and await it to finish.
    co_await background_executor->submit([file_path, file_content = std::move(processed_file_content)] {
        std::ofstream output;
        output.exceptions(std::ofstream::badbit);
        output.open(file_path, std::ios::binary);
        output.write(file_content.data(), file_content.size());
    });

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
