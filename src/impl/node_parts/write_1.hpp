#pragma once

#include "node_base.hpp"

#include "impl/silent_mt_queue.hpp"
#include "impl/work_counter.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"

#include "impl/edges/edge_out.hpp"

namespace dmn {

class node_impl_write_1: public virtual node_base_t {
    work_counter_t                  pending_writes_;

    using edge_t = edge_out_t<packet_network_t>;
    using link_t = edge_t::link_t;
    edge_t                          edge_;
    std::atomic<bool>               started_at_least_1_link_{false};

    void on_error(const boost::system::error_code& e, tcp_write_proto_t::guard_t&& guard) {
        auto& link = edge_t::link_from_guard(guard);
        auto p = std::move(link.packet);
        edge_.push_immediate(std::move(p));

        if (state() == node_state::RUN) {
            edge_.try_send();
            link.async_connect(std::move(guard)); // TODO: dealy?
            return;
        }

        pending_writes_.remove();
        link.close(std::move(guard));
        on_stop_writing();
        // TODO: log issue
    }

    void on_operation_finished(tcp_write_proto_t::guard_t&& guard) {
        started_at_least_1_link_.store(true, std::memory_order_relaxed);
        pending_writes_.remove();
        edge_.try_steal_work(std::move(guard));
    }

public:
    node_impl_write_1() {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );

        BOOST_ASSERT_MSG(edges_out.second - edges_out.first == 1, "Incorrect node class used for dealing single out edge. Error in make_node() function");
        const vertex_t& out_vertex = config[target(*edges_out.first, config)];

        const auto hosts_count = out_vertex.hosts.size();
        pending_writes_.add(hosts_count);
        edge_.preinit_links(hosts_count);
        for (std::size_t i = 0; i < hosts_count; ++ i) {
            edge_.inplace_construct_link(
                i,
                out_vertex.hosts[i].first.c_str(),
                out_vertex.hosts[i].second,
                ios(),
                [this](const auto& e, auto guard) { on_error(e, std::move(guard)); },
                [this](auto guard) { on_operation_finished(std::move(guard)); }
            );
        }
        edge_.connect_links();
    }

    void on_stop_writing() noexcept override final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept override final {
        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");

        edge_.assert_no_more_data();
        edge_.close_links();
    }

    void on_packet_accept(packet_t packet) override final {
        pending_writes_.add();

        packet_t data = call_callback(std::move(packet));

        edge_.push(packet_network_t{std::move(data)});

        // Rechecking for case when write operation was finished before we pushed into the data_to_send_
        edge_.try_send();
    }

    ~node_impl_write_1() noexcept {
        const bool soft_shutdown = started_at_least_1_link_.load();
        if (soft_shutdown) {
            edge_.assert_no_more_data();
            BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
        }

        edge_.close_links();
    }
};

}
