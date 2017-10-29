#pragma once

#include "node_base.hpp"

#include "netlink.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>

namespace dmn {

class node_impl_read_1: public node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;
    std::deque<netlink_ptr>         netlinks_;

    void on_accept(const boost::system::error_code& error) {
        if (error) {
            return;
        }

        netlinks_.push_back(
            netlink_t::construct(*this, std::move(new_socket_))
        );

        start_accept();
    }

    void start_accept() {
        acceptor_.async_accept(new_socket_, [this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

public:
    node_impl_read_1(std::istream& in, const char* node_id, std::size_t host_id)
        : node_base_t(in, node_id)
        , acceptor_(
                ios(),
                boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(), 63101  // TODO: port
                )
            )
        , new_socket_(ios())
    {}

    void start() override final {
        start_accept();
    }

    void on_packet_accept(packet_native_t&& data) override final {
        // Form a stream from message
        stream_t stream{*this, std::move(data)};
        callback_(stream);
        on_packet_send(stream.move_out_data());
    }

    ~node_impl_read_1() noexcept = default;
};

}
