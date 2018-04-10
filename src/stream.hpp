#pragma once

#include "node_base.hpp"
#include "impl/packet.hpp"

namespace dmn {

// Does the packet <--> user data conversions
class stream_t {
    DMN_PINNED(stream_t);

    node_base_t& node_;

    packet_t in_data_;
    packet_t out_data_;

public:
    explicit stream_t(node_base_t& node, packet_t&& in_data)
        : node_(node)
        , in_data_(std::move(in_data))
    {
        out_data_.place_header();
        out_data_.header().wave_id = in_data_.header().wave_id;
    }

    packet_t&& move_out_data() noexcept {
        return std::move(out_data_);
    }

    void add(const void* data, std::size_t size, const char* type) {
        if (!type) {
            type = "";
        }
        out_data_.add_data(static_cast<const unsigned char*>(data), size, type);
    }

    std::pair<const void*, std::size_t> get_data(const char* type) const noexcept {
        if (!type) {
            type = "";
        }
        return { in_data_.get_data(type) };
    }
};

}
