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

    boost::asio::io_service& ios() noexcept;

    virtual void add_message(const void* data, std::size_t size, const char* type = "") = 0;
    virtual void send_message() = 0;
};

}
