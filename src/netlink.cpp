#include "netlink.hpp"

#include "packet_network.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

namespace dmn {

namespace {
    inline packet_network_t& netpacket(std::aligned_storage_t<sizeof(packet_native_t), alignof(packet_native_t)>& data) noexcept {
        static_assert(sizeof(packet_native_t) == sizeof(packet_network_t), "");
        static_assert(alignof(packet_native_t) == alignof(packet_network_t), "");

        return *reinterpret_cast<packet_network_t*>(
            &data
        );
    }
}

void netlink_t::process_and_async_read() {
    auto data = std::move(native_in_);
    async_read();
    node_.on_packet_accept(std::move(data));
}

void netlink_t::async_read_data() {
    if (!native_in_.header_.size) {
        native_in_.body_.data_.clear();
        process_and_async_read();
        return;
    }

    native_in_.body_.data_.resize(native_in_.header_.size);
    const auto buf = netpacket(network_in_holder_).body_.buffer();
    boost::asio::async_read(
        socket_,
        buf,
        [this](const boost::system::error_code& e, std::size_t bytes_read) mutable {
            if (e) {
                return;
            }
            BOOST_ASSERT_MSG(bytes_read == sizeof(native_in_.header_.size), "Wrong bytes read for data");

            process_and_async_read();
        }
    );
}


netlink_t::netlink_t(node_base_t& node, boost::asio::ip::tcp::socket&& socket)
    : socket_(std::move(socket))
    , node_(node)
{
    new (&network_in_holder_) packet_network_t();
    new (&network_out_holder_) packet_network_t();
    async_read();
}

netlink_t::netlink_t(node_base_t& node, const char* addr)
    : socket_(node.ios())
    , node_(node)
{
    new (&network_in_holder_) packet_network_t();
    new (&network_out_holder_) packet_network_t();

    const boost::asio::ip::tcp::endpoint ep{
        boost::asio::ip::address_v4::from_string(addr),
        63101 // TODO: port number
    };
    socket_.connect(ep);

    async_read();
}

netlink_t::~netlink_t() {
    netpacket(network_out_holder_).~packet_network_t();
    netpacket(network_in_holder_).~packet_network_t();
}

void netlink_t::async_read() {
    const auto buf = netpacket(network_in_holder_).header_.buffer();
    boost::asio::async_read(
        socket_,
        buf,
        [this](const boost::system::error_code& e, std::size_t bytes_read) {
            if (e) {
                BOOST_ASSERT(false);
                return; // TODO:
            }

            native_in_.header_ = netpacket(network_in_holder_).header_.to_native();
            BOOST_ASSERT_MSG(bytes_read == sizeof(packet_header_network_t), "Wrong bytes read for header");

            switch (native_in_.header_.packet_type) {
            case packet_types_enum::DATA:
                async_read_data();
                return;
            default:
                BOOST_ASSERT_MSG(false, "Unknown packet type"); // TODO: log error, not assert
                return;
            };
        }
    );
}

void netlink_t::async_send(guart_t&& g, packet_native_t&& data) {
    BOOST_ASSERT(g);
    BOOST_ASSERT(g.mutex() == this);

    data.header_.packet_type = packet_types_enum::DATA;
    data.header_.size = data.body_.data_.size();

    netpacket(network_out_holder_) = packet_network_t(std::move(data));
    auto buf = netpacket(network_out_holder_).buffer();

    const std::size_t size = boost::asio::buffer_size(buf);

    boost::asio::async_write(
        socket_,
        buf,
        [guard = std::move(g), size](const boost::system::error_code& e, std::size_t bytes_written) {
            BOOST_ASSERT_MSG(size == bytes_written, "Wrong bytes count written");
            BOOST_ASSERT_MSG(!e, "Error occured");
        }
    );
}


netlink_t::guart_t netlink_t::try_lock() noexcept {
    int expected = 0;
    const bool res = write_lock_.compare_exchange_strong(expected, 1, std::memory_order_relaxed);

    if (res) {
        return {*this, std::defer_lock};
    }

    return {};
}

void netlink_t::unlock() noexcept {
    const auto old_value = write_lock_.fetch_sub(1);
    BOOST_ASSERT(old_value == 1);
}

} // namespace dmn
