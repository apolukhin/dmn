#pragma once

#include "node_base.hpp"

#include "impl/silent_mt_queue.hpp"
#include "impl/work_counter.hpp"

#include "impl/edges/edge_out.hpp"
#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"

#include <vector>
#include <boost/make_unique.hpp>

namespace dmn {

class counted_packets_storage {
    struct counted_packet {
        packet_network_t packet;
        std::size_t      count;
    };

    struct addr_comparator {
        inline bool operator()(const counted_packet& lhs, const counted_packet& rhs) const noexcept {
            return lhs.packet.data_address() < rhs.packet.data_address();
        }
        inline bool operator()(const counted_packet& lhs, const packet_network_t& rhs) const noexcept {
            return lhs.packet.data_address() < rhs.data_address();
        }
        inline bool operator()(const packet_network_t& lhs, const counted_packet& rhs) const noexcept {
            return lhs.data_address() < rhs.packet.data_address();
        }
        inline bool operator()(const counted_packet& lhs, boost::asio::const_buffer rhs) const noexcept {
            return boost::asio::buffer_cast<const void*>(lhs.packet.body_const_buffer()) < boost::asio::buffer_cast<const void*>(rhs);
        }
        inline bool operator()(boost::asio::const_buffer lhs, const counted_packet& rhs) const noexcept {
            return boost::asio::buffer_cast<const void*>(lhs) < boost::asio::buffer_cast<const void*>(rhs.packet.body_const_buffer());
        }
    };

    std::mutex                                          packets_mutex_;
    std::vector<counted_packet>                         packets_;
    std::size_t                                         pending_packets_{0};
    const std::size_t                                   counter_init_;

public:
    explicit counted_packets_storage(std::size_t counter_init)
        : counter_init_(counter_init)
    {}

    boost::asio::const_buffers_1 add_packet(packet_network_t p) {
        BOOST_ASSERT_MSG(!p.empty(), "Attempt to add an empty packet (even without header!)");
        if (p.expected_body_size() == 0) {
            return boost::asio::const_buffers_1{nullptr, 0}; // Do nothing
        }

        std::lock_guard<std::mutex> l(packets_mutex_);
        auto it = std::lower_bound(packets_.begin(), packets_.end(), p, addr_comparator{});
        it = packets_.insert(it, {std::move(p), counter_init_});
        pending_packets_ += counter_init_;
        return it->packet.body_const_buffer();
    }

    void send_success(boost::asio::const_buffer buf) {
        if (boost::asio::buffer_size(buf) == 0) {
            return;
        }

        std::lock_guard<std::mutex> l(packets_mutex_);
        const auto it = std::equal_range(packets_.begin(), packets_.end(), buf, addr_comparator{});
        BOOST_ASSERT_MSG(it.second != it.first, "No packet found after successful send");
        BOOST_ASSERT_MSG(it.second - it.first == 1, "Found more that one matching buffer after successful send");
        counted_packet& v = *it.first;
        const auto val = --v.count;
        --pending_packets_;
        if (val == 0) {
            packets_.erase(it.first);
        }
    }

    std::size_t pending_packets() noexcept {
        std::lock_guard<std::mutex> l(packets_mutex_);
        return pending_packets_;
    }
};

class node_impl_write_n: public virtual node_base_t {
    work_counter_t                  pending_writes_;

    using edge_t = edge_out_round_robin_t<std::pair<packet_header_t, boost::asio::const_buffer>>;
    using link_t = edge_t::link_t;

    const std::size_t               edges_count_;
    lazy_array<edge_t>              edges_;

    counted_packets_storage         packets_;

    std::atomic<bool>               started_at_least_1_link_{false};

    void reconnect_if_running(const boost::system::error_code& e, tcp_write_proto_t::guard_t guard) {
        BOOST_ASSERT_MSG(guard, "Empty guard in error handler");
        auto& link = edge_t::link_from_guard(guard);
        // TODO: async log issue

        if (state() != node_state::RUN) {
            // Drop the write task on the floor.
            pending_writes_.remove();
            link.close(std::move(guard));
            on_stop_writing();
            return;
        }

        link.async_reconnect(std::move(guard)); // TODO: dealy?
    }

    void on_send_error(const boost::system::error_code& e, tcp_write_proto_t::guard_t guard) {
        pending_writes_.add();

        const auto edge_id = edge_t::link_from_guard(guard).helper_id();
        edges_[edge_id].reschedule_packet_from_link(guard);

        reconnect_if_running(e, std::move(guard));
    }

    void on_operation_finished(tcp_write_proto_t::guard_t guard) {
        started_at_least_1_link_.store(true); // TODO: this is a debug thing. Remove it in release builds
        pending_writes_.remove();

        auto& link = edge_t::link_from_guard(guard);
        packets_.send_success(link.packet.second);
        const auto id = link.helper_id();
        edges_[id].try_steal_work(std::move(guard));
    }

public:
    node_impl_write_n()
        : edges_count_(count_out_edges())
        , packets_(edges_count_)
    {
        edges_.init(edges_count_);

        auto edges_it = boost::out_edges(
            this_node_descriptor,
            config
        ).first;

        for (std::size_t i = 0; i < edges_count_; ++i, ++edges_it) {
            const vertex_t& out_vertex = config[boost::target(*edges_it, config)];

            const auto hosts_count = out_vertex.hosts.size();
            pending_writes_.add(hosts_count);
            edges_.inplace_construct(i, edge_id_for_receiver(i));
            edges_[i].preinit_links(hosts_count);
            for (std::size_t j = 0; j < hosts_count; ++j) {
                const auto& host = out_vertex.hosts[j];
                edges_[i].inplace_construct_link(
                    j,
                    host.first.c_str(),
                    host.second,
                    ios(),
                    [this](const auto& e, auto guard, tcp_write_proto_t::send_error_tag) { on_send_error(e, std::move(guard)); },
                    [this](auto guard) { on_operation_finished(std::move(guard)); },
                    [this](const auto& e, auto guard, tcp_write_proto_t::reconnect_error_tag) { reconnect_if_running(e, std::move(guard)); },
                    i
                );
            }
            edges_[i].connect_links();
        }
    }

    void on_stop_writing() noexcept final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept  final {
        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
        BOOST_ASSERT_MSG(!packets_.pending_packets(), "Stopped writing in soft shutdown, but still have pending_packets");

        for (auto& edge: edges_) {
            edge.assert_no_more_data();
            edge.close_links();
        }
    }

    void on_packet_accept(packet_t packet) final {
        pending_writes_.add(edges_count_);

        BOOST_ASSERT_MSG(!packet.empty(), "Attempt to send an empty packet, even without a header");

        auto response_packet = call_callback(std::move(packet));
        const packet_header_t header = response_packet.header();
        packet_network_t data{ std::move(response_packet) };
        const auto body = packets_.add_packet(std::move(data));

        for (auto& edge: edges_) {
            auto header_cpy = header;
            header_cpy.edge_id = edge.edge_id_for_receiver(); // TODO: big/little endian

            edge.push(header.wave_id, {header_cpy, body});
        }
    }

    ~node_impl_write_n() noexcept override {
        const bool soft_shutdown = started_at_least_1_link_.load();

        for (auto& edge: edges_) {
            if (soft_shutdown) {
                edge.assert_no_more_data();
            }

            edge.close_links();
        }

        BOOST_ASSERT_MSG(!soft_shutdown || !pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
        BOOST_ASSERT_MSG(!soft_shutdown || !packets_.pending_packets(), "Stopped writing in soft shutdown, but still have pending_packets");
    }
};

}
