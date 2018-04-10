#pragma once

#include "node_base.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/edges/edge_in.hpp"
#include "impl/net/tcp_read_proto.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>

#include <mutex>

namespace dmn {

class node_impl_read_1: public virtual node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;

    using edge_t = edge_in_t<packet_network_t>;
    using link_t = edge_t::link_t;
    edge_t edge_;

    void on_error(link_t& link, const boost::system::error_code& e) {
        edge_.remove_link(link);
        // TODO: log issue
    }

    void on_accept(const boost::system::error_code& error) {
        if (error.value() == boost::asio::error::operation_aborted && ios().stopped()) {
            edge_.close_links();
            return;
        }
        BOOST_ASSERT_MSG(!error, "Error while accepting");

        auto link_ptr = link_t::construct(
            std::move(new_socket_),
            [this](auto& proto, const auto& e) { on_error(link_t::to_link(proto), e); },
            [this](auto& proto) { on_operation_finished(link_t::to_link(proto)); }
        );
        start_accept();

        auto& link = edge_.add_link(std::move(link_ptr));
        link.async_read(link.packet.header_mutable_buffer());
    }

    void on_operation_finished(link_t& link) {
        BOOST_ASSERT_MSG(link.packet.packet_type() == packet_types_enum::DATA, "Packet types other than DATA are not supported");
        if (link.packet.expected_body_size() != link.packet.actual_body_size()) {
            link.async_read(link.packet.body_mutable_buffer());
            return;
        }
        auto p = std::move(link.packet);

        link.async_read(link.packet.header_mutable_buffer());
        on_packet_accept(std::move(p).to_native());
    }

    void start_accept() {
        acceptor_.async_accept(new_socket_, [this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

public:
    node_impl_read_1()
        : acceptor_(ios())
        , new_socket_(ios())
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

    void single_threaded_io_detach_read() noexcept {
        acceptor_.close();
        edge_.close_links();
    }
};

}
