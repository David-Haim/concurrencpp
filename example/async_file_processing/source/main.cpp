/*
        In this example, we'll use concurrencpp executors to process a file
   asynchronously, the application iterate over the file characters (we assume
   ASCII) and replaces a character with another. The application gets three
   parameters through the command line arguments: argv[0] - the path to the
   binary that created this process (not used in this example) argv[1] - a file
   path argv[2] - the character to replace argv[3] - the character to replace
   with.

        Since standard file streams are blocking, we would like to execute
   file-io operations using the background_executor, who's job is to execute
   relatively short-blocking tasks (like file-io).

        Processing the file content is a cpu-bound task (iterating over a binary
   buffer and potentially changing characters), so after reading the file we
   will resume execution in the thread_pool_executor,

        After content has been modified, it is ready to be re-written back to
   the file. we will again schedule a blocking write operation to the
   background_executor.
*/

#include <iostream>
#include <vector>
#include <fstream>

#include "concurrencpp/concurrencpp.h"

concurrencpp::result<void> replace_chars_in_file(concurrencpp::runtime& runtime, const std::string file_path, char from, char to) {

    auto file_content_result = runtime.background_executor()->submit([file_path] {
        std::ifstream input;
        input.exceptions(std::ifstream::badbit);
        input.open(file_path, std::ios::binary);
        std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});
        input.close();
        return buffer;
    });

    // if we just await on the result returned by the background_executor, the
    // execution resumes inside the blocking-threadpool. it is important to resume
    // execution in the cpu-threadpool by calling result::await_via or
    // result::resolve_via.
    auto file_content = co_await file_content_result.await_via(runtime.thread_pool_executor());

    for (auto& c : file_content) {
        if (c == from) {
            c = to;
        }
    }

    // schedule the write operation on the background executor and await on it to
    // finish.
    co_await runtime.background_executor()->submit([file_path, file_content = std::move(file_content)] {
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
        replace_chars_in_file(runtime, file_path, from_char, to_char).get();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
