#pragma once

#include "node_base.hpp"

#include "message_header.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>

namespace dmn {

class message_t;

struct stream_t {
    boost::asio::ip::tcp::socket socket_;
    node_base_t& node_;
    message_header msg_header_;

    stream_t(stream_t&&) = delete;
    stream_t(const stream_t&) = delete;
    stream_t& operator=(stream_t&&) = delete;
    stream_t& operator=(const stream_t&) = delete;
};


using stream_ptr_t = std::unique_ptr<stream_t>;

stream_ptr_t connect_stream(node_base_t& node, const char* addr) {
    boost::asio::ip::tcp::endpoint ep{
        boost::asio::ip::address_v4::from_string(addr),
        63101 // TODO: port number
    };

    return stream_ptr_t(new stream_t{
        boost::asio::ip::tcp::socket{node.ios(), std::move(ep)},
        node
    });
}


void read_data(stream_ptr_t&& s);

void read_headers(stream_ptr_t&& s) {
    auto& soc = s->socket_;
    const auto buf = boost::asio::buffer(&s->msg_header_, sizeof(s->msg_header_));
    boost::asio::async_read(
        soc,
        buf,
        [s = std::move(s)](const boost::system::error_code& e, std::size_t bytes_read) mutable {
            if (e) {
                return;
            }
            BOOST_ASSERT_MSG(bytes_read == sizeof(s->msg_header_), "Wrong bytes read for header");

            switch (s->msg_header_.packet_type) {
            case packet_types_enum::DATA:
                read_data(std::move(s));
                return;
            default:
                BOOST_ASSERT_MSG(false, "Unknown packet type"); // TODO: log error, not assert
                return;
            };
        }
    );
}

void send_data(stream_ptr_t&& s, message_t&& m);
    BOOST_ASSERT_MSG(false, "TODO");
}
