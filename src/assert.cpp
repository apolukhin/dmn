#include <stdexcept>    // std::logic_error
#include <iostream>     // std::cerr
#include <boost/stacktrace.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
    void assertion_failed_msg(char const* expr, char const* msg, char const* function, char const* file, long line) {
        std::cerr << "Expression '" << expr << "' is false in function '" << function << "': " << (msg ? msg : "<...>") << ".\n"
            <<  " => " << file << ':' << line << '\n'
            << "Backtrace:\n" << boost::stacktrace::stacktrace() << '\n';
        std::abort();
    }

    void assertion_failed(char const* expr, char const* function, char const* file, long line) {
        ::boost::assertion_failed_msg(expr, 0 /*nullptr*/, function, file, line);
    }
} // namespace boost
