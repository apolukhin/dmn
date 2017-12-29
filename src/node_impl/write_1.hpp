#pragma once

#include "node_base.hpp"
#include "netlink.hpp"
#include "silent_mt_queue.hpp"
#include "work_counter.hpp"

#include "netlink_with_packet.hpp"
#include "edge_out.hpp"
#include "packet_network.hpp"


namespace dmn {

class node_impl_write_1: public virtual node_base_t {
    work_counter_t                  pending_writes_;
    edge_out_t<packet_network_t>    edge_;
    std::atomic<bool>               started_at_least_1_link_{false};

    void on_error(const boost::system::error_code& e, netlink_write_t::guard_t&& guard) {
        BOOST_ASSERT_MSG(guard, "Empy guard in error handler");
        auto& link = *guard.mutex();
        auto it = edge_.find_netlink(guard);
        auto p = std::move(it->packet);
        edge_.push_immediate(std::move(p));

        if (state() == node_state::RUN) {
            edge_.try_send();
            link.async_connect(std::move(guard)); // TODO: dealy?
            return;
        }

        pending_writes_.remove();
        link.async_close(std::move(guard));
        on_stop_writing();
        // TODO: log issue
    }

    void on_operation_finished(netlink_write_t::guard_t&& guard) {
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

        pending_writes_.add(out_vertex.hosts.size());
        edge_out_t<packet_network_t>::netlinks_t links;
        links.reserve(out_vertex.hosts.size());
        for (const auto& host : out_vertex.hosts) {

            auto link = netlink_write_t::construct(
                host.first.c_str(),
                host.second,
                ios(),
                [this](const auto& e, auto&& guard) { on_error(e, std::move(guard)); },
                [this](auto&& guard) { on_operation_finished(std::move(guard)); }
            );
            links.emplace_back(netlink_with_packet_t<packet_network_t>{std::move(link), packet_network_t{}});
        }
        edge_.set_links(std::move(links));
    }

    void on_stop_writing() noexcept override final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept override final {
        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");

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
        edge_.close_links(!soft_shutdown);
        BOOST_ASSERT_MSG(!soft_shutdown || !pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
    }
};

}
