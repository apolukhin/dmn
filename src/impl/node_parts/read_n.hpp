#pragma once

#include "node_base.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/edges/edge_in.hpp"
#include "impl/net/tcp_read_proto.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>
#include <boost/container/small_vector.hpp>

#include <mutex>

namespace dmn {

struct count_in_edges_helper: public virtual node_base_t {
    std::size_t count_in_edges() const noexcept {
        const auto edges_in = boost::in_edges(
            this_node_descriptor,
            config
        );
        const std::size_t edges_in_count = edges_in.second - edges_in.first;
        BOOST_ASSERT_MSG(edges_in_count > 1, "Incorrect node class used for dealing with muliple out edges. Error in make_node() function");

        return edges_in_count;
    }
};

class packets_gatherer_t {
    DMN_PINNED(packets_gatherer_t);

    std::mutex  packets_mutex_;
    using received_edge_ids_t = boost::container::small_vector<std::size_t, 16>;
    std::unordered_map<wave_id_t, std::pair<received_edge_ids_t, packet_network_t> > packets_gatherer_;

    const std::size_t edges_count_;

public:
    explicit packets_gatherer_t(std::size_t edge_count)
        : edges_count_(edge_count)
    {}

    boost::optional<packet_network_t> combine_packets(packet_network_t p) {
        boost::optional<packet_network_t> result;
        const auto wave_id = p.wave_id_from_packet();
        const auto edge_id = p.edge_id_from_packet();

        std::lock_guard<std::mutex> l(packets_mutex_);
        const auto it = packets_gatherer_.find(wave_id);
        if (it == packets_gatherer_.cend()) {
            auto packet_it = packets_gatherer_.emplace(
                wave_id,
                std::pair<received_edge_ids_t, packet_network_t>{{}, std::move(p)}
            ).first;
            packet_it->second.first.push_back(edge_id);
            return result;
        }

        auto& edge_counter = it->second.first;
        auto ins_it = std::lower_bound(edge_counter.begin(), edge_counter.end(), edge_id);
        if (ins_it != edge_counter.end() && *ins_it == edge_id) {
            // Duplicate packet.
            // TODO: Write to log
            return result;
        }

        edge_counter.insert(ins_it, edge_id);
        if (edge_counter.size() == edges_count_) {
            result.emplace(std::move(it->second.second));
            packets_gatherer_.erase(it);
            return result;
        }

        return result;
    }

    // TODO: drop partial packets after timeout
};

class node_impl_read_n: private count_in_edges_helper {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;

    using edge_t = edge_in_t<packet_network_t>;
    using link_t = edge_t::link_t;

    const std::size_t               edges_count_;
    const std::unique_ptr<edge_t[]> edges_;

    packets_gatherer_t packs_;


    template <class F>
    void for_each_edge(F f) {
        for (std::size_t i = 0; i < edges_count_; ++i) {
            f(edges_[i]);
        }
    }

    void on_error(link_t& link, const boost::system::error_code& e) {
        if (!link.is_helper_id_set()) {
            delete &link;           // Destroying non owned link
            // TODO: log issue
            return;
        }

        const auto links_count = edges_[link.get_helper_id()].remove_link(link);
        if (!links_count) {
            no_more_readers();
            return;
        }

        // TODO: log issue
    }

    void on_accept(const boost::system::error_code& error) {
        if (error.value() == boost::asio::error::operation_aborted && state() != node_state::RUN) {
            return;
        }
        BOOST_ASSERT_MSG(!error, "Error while accepting");

        auto link_ptr = link_t::construct(
            std::move(new_socket_),
            [this](auto& proto, const auto& e) { on_error(link_t::to_link(proto), e); },
            [this](auto& proto) { on_operation_finished(link_t::to_link(proto)); }
        );
        start_accept();

        link_ptr->packet.header_mutable_buffer();    // place_header() is actually required here

        // TODO: This is BAD! change the logic to avoid unowned pointers.
        auto* link = link_ptr.release();    // Releasing link ownership to reclaim it in `on_operation_finished` or delete it in `on_error`

        link->async_read(link->packet.header_mutable_buffer());
    }

    void on_operation_finished(link_t& link) {
        BOOST_ASSERT_MSG(link.packet.packet_type() == packet_types_enum::DATA, "Packet types other than DATA are not supported");
        if (!link.is_helper_id_set()) {
            std::unique_ptr<link_t> link_ptr(&link); // Taking ownership

            link.set_helper_id(link.packet.edge_id_from_packet());
            edges_[link.get_helper_id()].add_link(std::move(link_ptr));
        }

        if (link.packet.expected_body_size() != link.packet.actual_body_size()) {
            link.async_read(link.packet.body_mutable_buffer());
            return;
        }
        auto p = std::move(link.packet);
        link.async_read(link.packet.header_mutable_buffer());

        /* TODO: STOPPED HERE
        auto res = packs_.combine_packets(std::move(p));
        if (res) {
            on_packet_accept(std::move(*res).to_native());
        }*/
        on_packet_accept(std::move(p).to_native());
    }

    void start_accept() {
        acceptor_.async_accept(new_socket_, [this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

public:
    node_impl_read_n()
        : count_in_edges_helper()
        , acceptor_(
            ios(),
            boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string(
                    config[this_node_descriptor].hosts[host_id_].first.c_str()
                ),
                config[this_node_descriptor].hosts[host_id_].second
            )
        )
        , new_socket_(ios())
        , edges_count_(count_in_edges_helper::count_in_edges())
        , edges_(boost::make_unique<edge_t[]>(edges_count_))
        , packs_(edges_count_)
    {
        start_accept();
    }

    void on_stop_reading() noexcept override final {
        acceptor_.cancel();

        std::size_t links_count = 0;
        for_each_edge([&links_count](auto& e){
            links_count += e.close_links();
        });

        if (!links_count) {
            no_more_readers();
        }
    }

    ~node_impl_read_n() noexcept {
        acceptor_.cancel();

        std::size_t links_count = 0;
        for_each_edge([&links_count](auto& e){
            links_count += e.close_links();
        });

        BOOST_ASSERT_MSG(links_count == 0, "Not all the links exited before shutdown");
    };
};

}
