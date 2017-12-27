#pragma once

#include "node.hpp"

#include "netlink.hpp"

namespace dmn {

class node_impl_write_0: public virtual node_base_t {
    work_counter_t                  pending_writes_;

public:
    node_impl_write_0() {}

    void on_packet_accept(packet_t packet) override final {
        pending_writes_.add();

        call_callback(std::move(packet));

        if (pending_writes_.remove() == 0) {
            no_more_writers();
        }
    }

    void on_stop_writing() noexcept override final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept override final {
        BOOST_ASSERT(!pending_writes_.get());
    }

    ~node_impl_write_0() noexcept = default;
};

}
