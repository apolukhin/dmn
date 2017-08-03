#pragma once

#include <type_traits>
#include <iosfwd>

#include <boost/utility/string_view.hpp>

namespace boost { namespace asio {
    class io_service;
}}

namespace dmn {

class node_t {
    struct impl_t;
    std::aligned_storage<1024>::type data;

    impl_t& impl() noexcept {
        return *reinterpret_cast<impl_t*>(&data);
    }

    const impl_t& impl() const noexcept {
        return *reinterpret_cast<const impl_t*>(&data);
    }

public:
    node_t(std::istream& in, boost::string_view node_id);
    ~node_t() noexcept;

    node_t(const node_t&) = delete;
    node_t& operator=(const node_t&) = delete;

    boost::asio::io_service& ios() noexcept;
};

}
