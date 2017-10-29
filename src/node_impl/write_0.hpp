#pragma once

#include "node.hpp"

#include "netlink.hpp"

namespace dmn {

class node_impl_write_0: public virtual node_base_t {
public:
    node_impl_write_0(std::istream& in, const char* node_id)
        : node_base_t(in, node_id)
    {}

    void on_packet_send(packet_native_t&& /*stream*/) override final {
        // Do nothing
    }

    ~node_impl_write_0() noexcept = default;
};

}
