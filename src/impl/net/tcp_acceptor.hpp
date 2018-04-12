#pragma once

#include "utility.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/optional/optional.hpp>
#include <thread>

namespace dmn {

class tcp_acceptor {
    DMN_PINNED(tcp_acceptor);

private:
    struct internals {
        boost::asio::ip::tcp::acceptor  acceptor_;
        boost::asio::ip::tcp::socket    new_socket_;

        explicit internals(boost::asio::io_context& ios)
            : acceptor_{ios}
            , new_socket_{ios}
        {}
    };

    boost::optional<internals>  data_;
    const boost::asio::ip::tcp::endpoint endpoint_;

    void open() {
        data_->acceptor_.open(endpoint_.protocol());
        data_->acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        data_->acceptor_.bind(endpoint_);
        data_->acceptor_.listen();
    }

public:
    tcp_acceptor(boost::asio::io_context& ios, const char* host, unsigned short port)
        : data_{ios}
        , endpoint_{boost::asio::ip::address::from_string(host), port}
    {
        open();
    }
    ~tcp_acceptor() {
        BOOST_ASSERT_MSG(!data_, "Acceptor must be closed before destruction!");
    }

    boost::asio::ip::tcp::socket&& extract_socket() noexcept {
        return std::move(data_->new_socket_);
    }

    template <class F>
    void async_accept(F callback) {
        data_->acceptor_.async_accept(data_->new_socket_, std::move(callback));
    }

    void close() noexcept {
        boost::system::error_code ignore;
        data_->acceptor_.close(ignore);
        data_->new_socket_.close(ignore);
        data_.reset();
    }
};

} // namespace dmn
