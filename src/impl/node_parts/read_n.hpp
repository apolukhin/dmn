#pragma once

#include "node_base.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/edges/edge_in.hpp"
#include "impl/net/tcp_read_proto.hpp"
#include "impl/node_parts/packets_gatherer.hpp"
#include "impl/work_counter.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>


#include <mutex>

namespace dmn {

class node_impl_read_n: public virtual node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;

    using edge_t = edge_in_t<packet_network_t>;
    using link_t = edge_t::link_t;

    const std::size_t               edges_count_;
    const std::unique_ptr<edge_t[]> edges_;

    packets_gatherer_t packs_;

    class unknown_links_t {
        std::mutex              unknown_links_mutex_;
        std::vector<std::unique_ptr<link_t>>   unknown_links_;
    public:
        using link_ptr_t = std::unique_ptr<link_t>;

        link_ptr_t extract(const link_t& l) {
            link_ptr_t res{};

            {
                std::lock_guard<std::mutex> lock{unknown_links_mutex_};
                auto it = std::find_if(unknown_links_.begin(), unknown_links_.end(), [l_ptr = &l](const auto& ptr) { return ptr.get() == l_ptr; });
                if (it == unknown_links_.end()) {
                    return res;
                }

                res = std::move(*it);
                unknown_links_.erase(it);
            }

            return res;
        }

        void add(link_ptr_t l) {
            std::lock_guard<std::mutex> lock{unknown_links_mutex_};
            unknown_links_.push_back(std::move(l));
        }
        
        void close() {
            std::lock_guard<std::mutex> lock{unknown_links_mutex_};
            for (auto& l : unknown_links_) {
                l->close();
            }
        }
    } unknown_links_;


    template <class F>
    void for_each_edge(F f) {
        for (std::size_t i = 0; i < edges_count_; ++i) {
            f(edges_[i]);
        }
    }

    void on_error(link_t& link, const boost::system::error_code& e) {
        if (link.is_helper_id_set()) {
            edges_[link.get_helper_id()].remove_link(link);
        } else {
            // Destroying non owned link
            unknown_links_.extract(link);
        }

        // TODO: log issue
    }

    void on_accept(const boost::system::error_code& error) {
        if (error.value() == boost::asio::error::operation_aborted && state() != node_state::RUN) {
            unknown_links_.close();

            for_each_edge([](auto& e){
                e.close_links();
            });
            return;
        }
        BOOST_ASSERT_MSG(!error, "Error while accepting");

        auto link_ptr = link_t::construct(
            std::move(new_socket_),
            [this](auto& proto, const auto& e) { on_error(link_t::to_link(proto), e); },
            [this](auto& proto) { on_operation_finished(link_t::to_link(proto)); }
        );

        start_accept();

        auto* link = link_ptr.get();    // Releasing link ownership to reclaim it in `on_operation_finished` or delete it in `on_error`
        unknown_links_.add(std::move(link_ptr));

        link->async_read(link->packet.header_mutable_buffer());
    }

    void on_operation_finished(link_t& link) {
        BOOST_ASSERT_MSG(link.packet.packet_type() == packet_types_enum::DATA, "Packet types other than DATA are not supported");
        if (!link.is_helper_id_set()) {
            std::unique_ptr<link_t> link_ptr = unknown_links_.extract(link);; // Taking ownership

            link.set_helper_id(link.packet.edge_id_from_packet());
            edges_[link.get_helper_id()].add_link(std::move(link_ptr));
        }

        if (link.packet.expected_body_size() != link.packet.actual_body_size()) {
            link.async_read(link.packet.body_mutable_buffer());
            return;
        }
        auto p = std::move(link.packet);
        link.async_read(link.packet.header_mutable_buffer());

        auto res = packs_.combine_packets(std::move(p));
        if (res) {
            on_packet_accept(std::move(*res).to_native());
        }
    }

    void start_accept() {
        acceptor_.async_accept(new_socket_, [this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

public:
    node_impl_read_n()
        : acceptor_(ios())
        , new_socket_(ios())
        , edges_count_(count_in_edges())
        , edges_(boost::make_unique<edge_t[]>(edges_count_))
        , packs_(edges_count_)
    {
        const auto endpoint = boost::asio::ip::tcp::endpoint(
            boost::asio::ip::address::from_string(
                config[this_node_descriptor].hosts[host_id_].first.c_str()
            ),
            config[this_node_descriptor].hosts[host_id_].second
        );
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        start_accept();
    }

    void on_stop_reading() noexcept final {
        acceptor_.close();
    }

    ~node_impl_read_n() noexcept override {
        boost::system::error_code ignore;
        acceptor_.close(ignore);
        unknown_links_.close();

        std::size_t links_count = 0;
        for_each_edge([&links_count](auto& e){
            links_count += e.close_links();
        });

        BOOST_ASSERT_MSG(links_count == 0, "Not all the links exited before shutdown");
    }
};

}
