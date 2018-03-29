#pragma once

#include "node_base.hpp"
#include "impl/work_counter.hpp"

namespace dmn {

class node_impl_write_0: public virtual node_base_t {
    work_counter_t                  pending_writes_;

public:
    node_impl_write_0() {}

    void on_packet_accept(packet_t packet) final {
        pending_writes_.add();

        call_callback(std::move(packet));

        if (pending_writes_.remove() == 0) {
            no_more_writers();
        }
    }

    void on_stop_writing() noexcept final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept final {
        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
    }

    ~node_impl_write_0() noexcept override {
        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
    }

};

}
