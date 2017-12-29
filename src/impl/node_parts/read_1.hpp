#pragma once

#include "node_base.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/net/tcp_read_proto.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>

#include <mutex>

namespace dmn {

class node_impl_read_1: public virtual node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;

    std::mutex                          netlinks_mutex_;
    using link_t = netlink_t<packet_network_t, tcp_read_proto_t>;
    std::deque<
        std::unique_ptr<link_t>
    >   netlinks_;

    std::size_t close_links() noexcept {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        for (auto& v: netlinks_) {   // TODO: balancing
            v->close();
        }

        return netlinks_.size();
    }

    link_t& add_link(std::unique_ptr<link_t> link_ptr) {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        auto it = std::lower_bound(netlinks_.begin(), netlinks_.end(), link_ptr);
        return **netlinks_.insert(it, std::move(link_ptr));
    }

    std::size_t remove_link(link_t& link) {
        link.close();
        link.packet.clear();

        std::unique_lock<std::mutex> guard(netlinks_mutex_);

        const auto pair = std::equal_range(netlinks_.begin(), netlinks_.end(), link, link_t::compare_addrs_t{});
        BOOST_ASSERT_MSG(pair.second - pair.first != 0, "Netlink not found in the vector of known netlinks");
        netlinks_.erase(pair.first);

        return netlinks_.size();
    }

    void on_error(link_t& link, const boost::system::error_code& e) {
        const auto links_count = remove_link(link);
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

        auto& link = add_link(std::move(link_ptr));

        link.async_read(link.packet.header_mutable_buffer());

        start_accept();
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
        : acceptor_(
            ios(),
            boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string(
                    config[this_node_descriptor].hosts[host_id_].first.c_str()
                ),
                config[this_node_descriptor].hosts.front().second
            )
        )
        , new_socket_(ios())
    {
        start_accept();
    }

    void on_stop_reading() noexcept override final {
        acceptor_.cancel();

        const auto links_count = close_links();
        if (!links_count) {
            no_more_readers();
        }
    }

    ~node_impl_read_1() noexcept {
        acceptor_.cancel();
        const auto links_count = close_links();
        BOOST_ASSERT_MSG(links_count == 0, "Not all the links exited before shutdown");
    };
};

}
