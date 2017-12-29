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
            return lhs.packet.data_address() < boost::asio::buffer_cast<const void*>(rhs);
        }
        inline bool operator()(boost::asio::const_buffer lhs, const counted_packet& rhs) const noexcept {
            return boost::asio::buffer_cast<const void*>(lhs) < rhs.packet.data_address();
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

        std::lock_guard<std::mutex> l(packets_mutex_);
        auto it = std::lower_bound(packets_.begin(), packets_.end(), p, addr_comparator{});
        it = packets_.insert(it, {std::move(p), counter_init_});
        pending_packets_ += counter_init_;
        return it->packet.const_buffer();
    }

    void send_success(boost::asio::const_buffer buf) {
        if (boost::asio::buffer_size(buf) == 0) {
            return;
        }

        std::lock_guard<std::mutex> l(packets_mutex_);
        const auto it = std::equal_range(packets_.begin(), packets_.end(), buf, addr_comparator{});
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

struct count_out_edges_helper: public virtual node_base_t {
    std::size_t count_out_edges() const noexcept {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );
        const std::size_t edges_out_count = edges_out.second - edges_out.first;
        BOOST_ASSERT_MSG(edges_out_count > 1, "Incorrect node class used for dealing with muliple out edges. Error in make_node() function");

        return edges_out_count;
    }
};

class node_impl_write_n: public count_out_edges_helper {
    work_counter_t                  pending_writes_;

    using edge_t = edge_out_t<boost::asio::const_buffer>;
    using link_t = edge_t::link_t;

    const std::size_t               edges_count_;
    const std::unique_ptr<edge_t[]> edges_;

    counted_packets_storage         packets_;

    std::atomic<bool>               started_at_least_1_link_{false};

    template <class F>
    void for_each_edge(F f) {
        for (std::size_t i = 0; i < edges_count_; ++i) {
            f(edges_[i]);
        }
    }

    void on_error(const boost::system::error_code& e, tcp_write_proto_t::guard_t&& guard) {
        BOOST_ASSERT_MSG(guard, "Empty guard in error handler");
        auto& link = edge_t::link_from_guard(guard);
        auto p = std::move(link.packet);
        auto& edge = edges_[link.helper_id()];
        edge.push_immediate(std::move(p));

        if (state() == node_state::RUN) {
            edge.try_send();
            link.async_connect(std::move(guard)); // TODO: dealy?
            return;
        }

        link.close(std::move(guard));
        // TODO: log issue
    }

    void on_operation_finished(tcp_write_proto_t::guard_t&& guard) {
        started_at_least_1_link_.store(true, std::memory_order_relaxed);
        pending_writes_.remove();

        auto& link = edge_t::link_from_guard(guard);
        packets_.send_success(link.packet);
        const auto id = link.helper_id();
        edges_[id].try_steal_work(std::move(guard));
    }

public:
    node_impl_write_n()
        : count_out_edges_helper{}
        , edges_count_(count_out_edges_helper::count_out_edges())
        , edges_(boost::make_unique<edge_t[]>(edges_count_))
        , packets_(edges_count_)
    {
        auto edges_it = boost::out_edges(
            this_node_descriptor,
            config
        ).first;

        for (std::size_t i = 0; i < edges_count_; ++i, ++edges_it) {
            const vertex_t& out_vertex = config[target(*edges_it, config)];

            const auto hosts_count = out_vertex.hosts.size();
            pending_writes_.add(hosts_count);
            edges_[i].preinit_links(hosts_count);
            for (std::size_t j = 0; j < hosts_count; ++j) {
                const auto& host = out_vertex.hosts[j];
                edges_[i].inplace_construct_link(
                    j,
                    host.first.c_str(),
                    host.second,
                    ios(),
                    [this](const auto& e, auto guard) { on_error(e, std::move(guard)); },
                    [this](auto guard) { on_operation_finished(std::move(guard)); },
                    i
                );
            }
            edges_[i].connect_links();
        }
    }

    void on_stop_writing() noexcept override final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept override final {
        BOOST_ASSERT_MSG(!pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
        BOOST_ASSERT_MSG(!packets_.pending_packets(), "Stopped writing in soft shutdown, but still have pending_packets");

        for_each_edge([](auto& edge){
            edge.assert_no_more_data();
            edge.close_links();
        });
    }

    void on_packet_accept(packet_t packet) override final {
        pending_writes_.add(edges_count_);

        packet_network_t data{ call_callback(std::move(packet)) };
        const auto buf = packets_.add_packet(std::move(data));

        for_each_edge([buf](auto& edge) {
            edge.push(buf);
            // Rechecking for case when write operation was finished before we pushed into the data_to_send_
            edge.try_send();
        });
    }

    ~node_impl_write_n() noexcept  {
        const bool soft_shutdown = started_at_least_1_link_.load();

        for_each_edge([soft_shutdown](edge_t& edge) {
            if (soft_shutdown) {
                edge.assert_no_more_data();
            }

            edge.close_links();
        });

        BOOST_ASSERT_MSG(!soft_shutdown || !pending_writes_.get(), "Stopped writing in soft shutdown, but still have pending_writes_");
        BOOST_ASSERT_MSG(!soft_shutdown || !packets_.pending_packets(), "Stopped writing in soft shutdown, but still have pending_packets");
    }
};

}
