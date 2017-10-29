#pragma once

#include <boost/container/flat_map.hpp>
#include "packet.hpp"

namespace dmn {

class node_base_t;

// Does the packet <--> user data conversions
class stream_t {
    node_base_t& node_;

    packet_native_t in_data_;
    packet_native_t out_data_;

public:
    explicit stream_t(node_base_t& node)
        : node_(node)
    {}

    explicit stream_t(node_base_t& node, packet_native_t&& in_data)
        : node_(node)
        , in_data_(std::move(in_data))
    {}

    packet_native_t&& move_out_data() noexcept {
        return std::move(out_data_);
    }

    void add(const void* data, std::size_t size, const char* type) {
        if (!type) {
            type = "";
        }

        BOOST_ASSERT(false);
    }
};

}
