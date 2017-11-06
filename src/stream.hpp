#pragma once

#include <boost/container/flat_map.hpp>
#include "packet.hpp"
#include "node_base.hpp"

namespace dmn {

// Does the packet <--> user data conversions
class stream_t {
    node_base_t& node_;
    stop_enum& stopping_;

    packet_native_t in_data_;
    packet_native_t out_data_;

public:
    explicit stream_t(node_base_t& node, packet_native_t&& in_data, stop_enum& stopping)
        : node_(node)
        , in_data_(std::move(in_data))
        , stopping_(stopping)
    {}

    void stop() {
        stopping_ = stop_enum::STOPPING_READ;
    }

    packet_native_t&& move_out_data() noexcept {
        return std::move(out_data_);
    }

    void add(const void* data, std::size_t size, const char* type) {
        if (!type) {
            type = "";
        }
        out_data_.body_.add_data(static_cast<const unsigned char*>(data), size, type);
    }

    std::pair<const void*, std::size_t> get_data(const char* type) const noexcept {
        if (!type) {
            type = "";
        }
        return { in_data_.body_.get_data(type) };
    }
};

}
