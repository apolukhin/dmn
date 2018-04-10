#pragma once

#include "node_base.hpp"
#include "impl/work_counter.hpp"

namespace dmn {

class node_impl_write_0: public virtual node_base_t {
public:
    node_impl_write_0() {}

    void on_packet_accept(packet_t packet) final {
        call_callback(std::move(packet));
    }

    void single_threaded_io_detach_write() noexcept {}
};

}
