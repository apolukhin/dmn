#pragma once

#include "node_base.hpp"
#include "netlink.hpp"

#include <mutex>

namespace dmn {

class node_impl_write_1: public virtual node_base_t {
    std::mutex                  netlinks_mutex_;
    std::deque<netlink_ptr>     netlinks_;

    netlink_t::guart_t try_get_netlink_or_reschedule() {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        for (auto& link: netlinks_) {
            netlink_t::guart_t lock = link->try_lock();
            if (lock) {
                return lock;
            }
        }

        return {};
    }

public:
    node_impl_write_1(std::istream& in, const char* node_id)
        : node_base_t(in, node_id)
    {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );

        BOOST_ASSERT(edges_out.second - edges_out.first == 1);
        const vertex_t& out_vertex = config[target(*edges_out.first, config)];

        for (const auto& host : out_vertex.hosts) {
            netlinks_.emplace_back(netlink_t::construct(*this, host.c_str()));
        }
    }


    void on_packet_send(packet_native_t&& data) override final {
        auto l = try_get_netlink_or_reschedule();
        if (!l) {
            // Reschedule
            ios().post(
                [this, m = std::move(data)]() mutable {
                    on_packet_send(std::move(m));
                }
            );
            return;
        }

        auto& link = *l.mutex();
        link.async_send(std::move(l), std::move(data));
    }

    ~node_impl_write_1() noexcept = default;
};

}
