#pragma once

#include "node.hpp"

#include "netlink.hpp"

namespace dmn {

class node_impl_write_0: public virtual node_base_t {
public:
    node_impl_write_0() {}

    void on_packet_send(write_ticket_t&& ticket, packet_native_t&& /*packet*/) override final {
        ticket.unlock();
    }

    void stop_writing() override final {
        state_.store(stop_enum::STOPPED, std::memory_order_relaxed);
    }

    ~node_impl_write_0() noexcept = default;
};

}
