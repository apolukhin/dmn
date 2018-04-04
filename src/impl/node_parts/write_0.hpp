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

    ~node_impl_write_0() noexcept override {
//        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
    }

};

}
