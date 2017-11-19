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

    std::mutex                      netlinks_mutex_;
    std::deque<netlink_read_ptr>    netlinks_;

    void on_error(const netlink_read_t* link, const boost::system::error_code& e) {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        if (netlinks_.empty()) {
            no_more_readers();
            return;
        }

        auto it = std::find_if(netlinks_.begin(), netlinks_.end(), [link](auto& v){
            return v.get() == link;
        });
        BOOST_ASSERT(it != netlinks_.end());

        auto l = std::move(*it);
        netlinks_.erase(it);
        guard.unlock();

        (void)l; // TODO: log issue
    }

    void on_accept(const boost::system::error_code& error) {
        if (error.value() == boost::asio::error::operation_aborted && state() != node_state::RUN) {
            return;
        }
        BOOST_ASSERT(!error);

        auto link = netlink_read_t::construct(
            std::move(new_socket_),
            [this](auto* p, const auto& e) { on_error(p, e); },
            [this](auto&& d) { on_packet_accept(std::move(d)); }
        );

        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        netlinks_.push_back(std::move(link));
        guard.unlock();

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

    void on_stop_reading() noexcept override final {
        acceptor_.cancel();

        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        std::for_each(netlinks_.begin(), netlinks_.end(), [](auto& v) {
            v->cancel();
        });

        if (netlinks_.empty()) {
            no_more_readers();
        }
    }

    ~node_impl_read_1() noexcept = default;
};

}
