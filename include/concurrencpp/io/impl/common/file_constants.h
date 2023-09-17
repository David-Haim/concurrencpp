#ifndef CONCURRENCPP_FILE_CONSTANTS_H
#define CONCURRENCPP_FILE_CONSTANTS_H

#include <ios>

namespace concurrencpp {
    enum class file_open_mode {
        read = std::ios_base::in,
        write = std::ios_base::out,
        append = std::ios_base::app,  //	seek to the end of stream before each write
        truncate = std::ios_base::trunc,  //  discard the contents of the stream when opening
        at_end = std::ios_base::ate  // seek to the end of stream immediately after open
    };

    enum class seek_direction { from_current, from_start, from_end };

    inline file_open_mode operator|(file_open_mode a, file_open_mode b) noexcept {
        return static_cast<file_open_mode>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline file_open_mode operator&(file_open_mode a, file_open_mode b) noexcept {
        return static_cast<file_open_mode>(static_cast<int>(a) & static_cast<int>(b));
    }
}  // namespace concurrencpp

#endif