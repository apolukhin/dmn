#pragma once

#include "node_base.hpp"
#include "netlink.hpp"

#include <mutex>

namespace dmn {

class node_impl_write_1: public virtual node_base_t {
    std::mutex                      netlinks_mutex_;
    std::deque<netlink_write_ptr>   netlinks_;

    netlink_write_t::guard_t try_get_netlink() {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        for (auto& link: netlinks_) {
            netlink_write_t::guard_t lock = link->try_lock();
            if (lock) {
                return lock;
            }
        }

        return {};
    }

public:
    node_impl_write_1() {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );

        BOOST_ASSERT(edges_out.second - edges_out.first == 1);
        const vertex_t& out_vertex = config[target(*edges_out.first, config)];

        for (const auto& host : out_vertex.hosts) {
            netlinks_.emplace_back(netlink_write_t::construct(*this, host.first.c_str(), host.second));
        }
    }

    virtual void stop_writing() override final {
        state_.store(stop_enum::STOPPING_WRITE, std::memory_order_relaxed);
        // TODO:
        // Sending SHUTDOWN_GRACEFULLY to all the links after there's no outstanding work
    }

    void on_packet_send(write_ticket_t&& ticket, packet_native_t&& data) override final {
        auto l = try_get_netlink();
        if (!l) {
            // Reschedule
            ios().post(
                [this, m = std::move(data), ticket = std::move(ticket)]() mutable {
                    on_packet_send(std::move(ticket), std::move(m));
                }
            );
            return;
        }

        auto& link = *l.mutex();
        link.async_send(std::move(ticket), std::move(l), std::move(data));    }

    ~node_impl_write_1() noexcept = default;
};

}
