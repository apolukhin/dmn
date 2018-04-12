#pragma once

#include "node_base.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/edges/edge_in.hpp"
#include "impl/net/tcp_acceptor.hpp"
#include "impl/net/tcp_read_proto.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>

#include <mutex>

namespace dmn {

class node_impl_read_1: public virtual node_base_t {
    tcp_acceptor acceptor_;

    using edge_t = edge_in_t<packet_network_t>;
    using link_t = edge_t::link_t;
    edge_t edge_;

    void on_error(link_t& link, const boost::system::error_code& e) {
        edge_.remove_link(link);
        // TODO: log issue
    }

    void on_accept(const boost::system::error_code& error) {
        BOOST_ASSERT_MSG(!error, "Error while accepting");

        auto link_ptr = link_t::construct(
            acceptor_.extract_socket(),
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
        acceptor_.async_accept([this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

public:
    node_impl_read_1()
        : acceptor_(ios(), config[this_node_descriptor].hosts[host_id_].first.c_str(), config[this_node_descriptor].hosts[host_id_].second)
    {
        start_accept();
    }

    void single_threaded_io_detach_read() noexcept {
        acceptor_.close();
        edge_.close_links();
    }
};

}
