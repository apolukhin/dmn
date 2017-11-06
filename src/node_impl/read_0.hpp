#pragma once

#include "node_base.hpp"
#include "packet.hpp"
#include "stream.hpp"
#include "write_work.hpp"

#include <boost/asio/io_service.hpp>

namespace dmn {

class node_impl_read_0: public virtual node_base_t {
    void start(read_ticket_t&& ticket) {
        ios().post([this, ticket = std::move(ticket)]() mutable {
            const stop_enum state = on_packet_accept(write_counter_.ticket(), packet_native_t{});
            if (state == stop_enum::RUN) {
                start(std::move(ticket));
            } else {
                if (ticket.release()->unlock_last()) {
                    stop_writing();
                }
            }
        });
    }
public:
    node_impl_read_0() {
        start(read_counter_.ticket()); // TODO: mutithreaded run
    }

    void stop_reading() override {
        state_.store(stop_enum::STOPPING_READ, std::memory_order_relaxed);
    }

    ~node_impl_read_0() noexcept = default;
};

}
