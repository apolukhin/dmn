#pragma once

#include "node_base.hpp"

#include "impl/silent_mt_queue.hpp"
#include "impl/work_counter.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/interval_timer.hpp"
#include "impl/net/packet_network.hpp"

#include "impl/edges/edge_out.hpp"

namespace dmn {

class node_impl_write_1: public virtual node_base_t {
    using edge_t = edge_out_round_robin_t<packet_network_t>;
    using link_t = edge_t::link_t;
    edge_t                          edge_;
    interval_timer                  timer_;

    void reconnect(const boost::system::error_code& e, tcp_write_proto_t::guard_t guard) {
        BOOST_ASSERT_MSG(guard, "Empty guard in error handler");

        // TODO: async log issue

        auto& link = edge_t::link_from_guard(guard);
        if (link.stability() > -5) {
            link.async_reconnect(std::move(guard));
        } else {
            timer_.async_wait([guard = std::move(guard)]() mutable {
                auto& link = edge_t::link_from_guard(guard);
                link.async_reconnect(std::move(guard));
            });
        }
    }

    void on_send_error(const boost::system::error_code& e, tcp_write_proto_t::guard_t guard) {
        edge_.reschedule_packet_from_link(guard);
        reconnect(e, std::move(guard));
    }

    void on_operation_finished(tcp_write_proto_t::guard_t guard) {
        edge_.try_steal_work(std::move(guard));
    }

public:
    node_impl_write_1()
        : edge_(edge_id_for_receiver())
        , timer_(ios(), std::chrono::milliseconds(50))
    {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );

        BOOST_ASSERT_MSG(edges_out.second - edges_out.first == 1, "Incorrect node class used for dealing single out edge. Error in make_node() function");
        const vertex_t& out_vertex = config[target(*edges_out.first, config)];

        const auto hosts_count = out_vertex.hosts.size();
        edge_.preinit_links(hosts_count);
        for (std::size_t i = 0; i < hosts_count; ++ i) {
            edge_.inplace_construct_link(
                i,
                out_vertex.hosts[i].first.c_str(),
                out_vertex.hosts[i].second,
                ios(),
                [this](const auto& e, auto guard, tcp_write_proto_t::send_error_tag) { on_send_error(e, std::move(guard)); },
                [this](auto guard) { on_operation_finished(std::move(guard)); },
                [this](const auto& e, auto guard, tcp_write_proto_t::reconnect_error_tag) { reconnect(e, std::move(guard)); }
            );
        }
        edge_.connect_links();
    }

    void on_packet_accept(packet_t packet) final {
        packet_t data = call_callback(std::move(packet));
        const auto wave_id = data.header().wave_id;
        data.header().edge_id = edge_.edge_id_for_receiver();

        edge_.push(wave_id, packet_network_t{std::move(data)});
    }

    void single_threaded_io_detach_write() noexcept {
        timer_.close();
        edge_.close_links();
    }
};

}
