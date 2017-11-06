#pragma once

#include <type_traits>
#include <iosfwd>

namespace boost { namespace asio {
    class io_service;
}}

namespace dmn {

class node_t {
protected:
    node_t() noexcept = default;
    ~node_t() noexcept;

public:
    node_t(const node_t&) = delete;
    node_t& operator=(const node_t&) = delete;

    static boost::asio::io_service& ios() noexcept;
};

}
