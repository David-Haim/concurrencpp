#ifndef CONCURRENCPP_IO_CANCELLED_H
#define CONCURRENCPP_IO_CANCELLED_H

#include <stdexcept>

namespace concurrencpp {
    struct io_cancelled : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}  // namespace concurrencpp

#endif  //  CONCURRENCPP_IO_CANCELLED_H
