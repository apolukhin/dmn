#pragma once

#include "node_base.hpp"

#include "stream.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace dmn {

class node_impl_1_x: public node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;

    void on_accept(const boost::system::error_code& error) {
        if (error) {
            return;
        }

        stream_ptr_t s (new stream_t{
            std::move(new_socket_),
            *this
        });

        start_accept();
        read_headers(std::move(s));
    }

    void start_accept() {
        acceptor_.async_accept(new_socket_, [this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

public:
    node_impl_1_x(std::istream& in, const char* node_id, std::size_t host_id)
        : node_base_t(in, node_id)
        , acceptor_(
                ios(),
                boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(), 63101  // TODO: port
                )
            )
        , new_socket_(ios())
    {
        start_accept();
    }

    ~node_impl_1_x() noexcept = default;
};

}
