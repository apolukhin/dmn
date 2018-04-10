#pragma once

#include <type_traits>
#include <iosfwd>

namespace boost { namespace asio {
    class io_context;
    using io_service = io_context;
}}

namespace dmn {

class node_t {
    boost::asio::io_service* ios_;

protected:
    node_t(boost::asio::io_service& ios) noexcept : ios_(&ios) {}
    ~node_t() noexcept;

public:
    node_t(const node_t&) = delete;
    node_t& operator=(const node_t&) = delete;

    boost::asio::io_service& ios() noexcept;
};

}
