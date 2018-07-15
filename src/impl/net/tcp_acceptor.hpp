#pragma once

#include "utility.hpp"
#include "impl/saturation_timer.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/make_unique.hpp>
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
    saturation_timer_t instability_;

    void try_open() {
        boost::system::error_code ignore;
        try_open(ignore);
    }

    void try_open(boost::system::error_code& er) {
        data_->acceptor_.open(endpoint_.protocol(), er);
        if (er) {
            return;
        }

        data_->acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        data_->acceptor_.bind(endpoint_, er);
        if (er) {
            data_->acceptor_.close();
            return;
        }
        data_->acceptor_.listen();
    }

public:
    tcp_acceptor(boost::asio::io_context& ios, const char* host, unsigned short port)
        : data_{ios}
        , endpoint_{boost::asio::ip::address::from_string(host), port}
    {
        try_open();
    }
    ~tcp_acceptor() {
        BOOST_ASSERT_MSG(!data_, "Acceptor must be closed before destruction!");
    }

    boost::asio::ip::tcp::socket&& extract_socket() noexcept {
        BOOST_ASSERT_MSG(data_, "Acceptor is not initialized!");
        BOOST_ASSERT_MSG(data_->acceptor_.is_open(), "Acceptor is not opened!");
        return std::move(data_->new_socket_);
    }

    template <class F>
    void async_accept(F callback) {
        if (data_->acceptor_.is_open()) {
            -- instability_;
            data_->acceptor_.async_accept(data_->new_socket_, std::move(callback));
        } else {
            ++instability_;

            auto timer_ptr = boost::make_unique<boost::asio::steady_timer>(data_->acceptor_.get_io_context());
            auto& timer = *timer_ptr;
            timer.expires_after(instability_.timeout());
            timer.async_wait([this, f = std::move(callback), t = std::move(timer_ptr)](boost::system::error_code ec) mutable {
                try_open(ec);
                if (!ec) {
                    data_->acceptor_.async_accept(data_->new_socket_, std::move(f));
                } else {
                    if (!instability_.is_max()) {
                        async_accept(std::move(f));
                    } else {
                        f(ec);
                    }
                }
            });
        }
    }

    void close() noexcept {
        boost::system::error_code ignore;
        data_->acceptor_.close(ignore);
        data_->new_socket_.close(ignore);
        data_.reset();
    }
};

} // namespace dmn
