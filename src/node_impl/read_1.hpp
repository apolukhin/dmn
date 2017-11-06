#pragma once

#include "node_base.hpp"

#include "netlink.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>

namespace dmn {

class node_impl_read_1: public virtual node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;
    std::deque<netlink_read_ptr>    netlinks_;

    std::atomic<stop_enum> state_ {stop_enum::RUN};

    void on_accept(const boost::system::error_code& error) {
        if (error.value() == boost::asio::error::operation_aborted & state_ != stop_enum::RUN) {
            return;
        }
        BOOST_ASSERT(!error);

        netlinks_.push_back(
            netlink_read_t::construct(*this, std::move(new_socket_), read_counter_.ticket())
        );

        start_accept();
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
                    config[this_node_descriptor].hosts.front().first.c_str() // TODO: not only front()!
                ),
                config[this_node_descriptor].hosts.front().second
            )
        )
        , new_socket_(ios())
    {
        start_accept();
    }

    void stop_reading() override final {
        acceptor_.cancel();
        state_.store(stop_enum::STOPPING_READ, std::memory_order_relaxed);
        // writers must send the SHUTDOWN_GRACEFULLY packets to us
    }

    ~node_impl_read_1() noexcept = default;
};

}
