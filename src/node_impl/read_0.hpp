#pragma once

#include "node_base.hpp"
#include "packet.hpp"
#include "stream.hpp"
#include "work_counter.hpp"

#include <boost/asio/io_service.hpp>

namespace dmn {

class node_impl_read_0: public virtual node_base_t {
    void start() {
        ios().post([this]() {
            on_packet_accept(packet_native_t{});
            if (state() == node_state::RUN) {
                start();
            } else {
                no_more_readers(); // TODO: mutithreaded run
            }
        });
    }
public:
    node_impl_read_0() {
        start(); // TODO: mutithreaded run
    }

    void on_stop_reading() noexcept override final {
        // Noop
    }

    ~node_impl_read_0() noexcept = default;
};

}
