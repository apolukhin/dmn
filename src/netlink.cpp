#include "netlink.hpp"

#include "packet_network.hpp"

#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <iostream>

namespace dmn {

namespace {
    inline packet_network_t& netpacket(std::aligned_storage_t<sizeof(packet_native_t), alignof(packet_native_t)>& data) noexcept {
        static_assert(sizeof(packet_native_t) == sizeof(packet_network_t), "");
        static_assert(alignof(packet_native_t) == alignof(packet_network_t), "");

        return *reinterpret_cast<packet_network_t*>(
            &data
        );
    }

    void process_error(const boost::system::error_code& e) {
        switch (e.value()) {
        case boost::asio::error::operation_aborted:
            // TODO: log me
            return;
        default:
            std::cerr << "process_error: " << e.message() << '\n';
            BOOST_ASSERT(false);
        }
    }
}

void netlink_read_t::process_and_async_read() {
    auto data = netpacket(network_in_holder_).to_native();
    auto write_ticket = node_.write_counter_.ticket();
    async_read();
    node_.on_packet_accept(std::move(write_ticket), std::move(data));
}

void netlink_read_t::async_read_data() {
    if (!native_in_.header_.size) {
        native_in_.body_.data_.clear();
        process_and_async_read();
        return;
    }

    const auto buf = netpacket(network_in_holder_).body_.write_buffer(native_in_.header_.size);
    boost::asio::async_read(
        socket_,
        buf,
        [this](const boost::system::error_code& e, std::size_t bytes_read) {
            if (e) {
                process_error(e);
                return;
            }
            BOOST_ASSERT_MSG(bytes_read == native_in_.header_.size, "Wrong bytes read for data");

            process_and_async_read();
        }
    );
}


netlink_read_t::netlink_read_t(node_base_t& node, boost::asio::ip::tcp::socket&& socket, read_ticket_t&& ticket)
    : socket_(std::move(socket))
    , node_(node)
    , ticket_(std::move(ticket))
{
    new (&network_in_holder_) packet_network_t();
    async_read();
}

netlink_write_t::netlink_write_t(node_base_t& node, const char* addr, unsigned short port)
    : socket_(node.ios())
    , node_(node)
    , remote_ep_{boost::asio::ip::address::from_string(addr), port}
{
    new (&network_out_holder_) packet_network_t();

    async_connect(try_lock());
}

netlink_read_t::~netlink_read_t() {
    netpacket(network_in_holder_).~packet_network_t();
}

netlink_write_t::~netlink_write_t() {
    netpacket(network_out_holder_).~packet_network_t();
}

void netlink_write_t::async_connect(netlink_write_t::guard_t&& g) {
    BOOST_ASSERT(g);
    socket_.async_connect(remote_ep_, [guard = std::move(g), this](const boost::system::error_code& e) mutable {;
        if (e.value() == boost::asio::error::connection_refused) {
            socket_.close();
            async_connect(std::move(guard)); // TODO: retries
            return;
        } else if (e) {
            process_error(e);
            return;
        }

        guard.unlock();
    });
}

void netlink_read_t::async_read() {
    const auto buf = netpacket(network_in_holder_).header_.read_buffer();
    boost::asio::async_read(
        socket_,
        buf,
        [this](const boost::system::error_code& e, std::size_t bytes_read) {
            if (e) {
                process_error(e);
                return;
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

void netlink_write_t::async_send(write_ticket_t&& ticket, guard_t&& g, packet_native_t&& data) {
    BOOST_ASSERT(g);
    BOOST_ASSERT(g.mutex() == this);

    data.header_.packet_type = packet_types_enum::DATA;
    data.header_.size = data.body_.data_.size();

    netpacket(network_out_holder_) = packet_network_t(std::move(data));
    auto buf = netpacket(network_out_holder_).read_buffer();

    const std::size_t size = boost::asio::buffer_size(buf);

    boost::asio::async_write(
        socket_,
        buf,
        [guard = std::move(g), ticket=std::move(ticket), size, this](const boost::system::error_code& e, std::size_t bytes_written) mutable {
            BOOST_ASSERT_MSG(size == bytes_written, "Wrong bytes count written");

            if (ticket.release()->unlock_last() && node_.state_ == stop_enum::STOPPING_WRITE) {

            }

            if (e) {
                process_error(e);
                return;
            }
        }
    );
}


netlink_write_t::guard_t netlink_write_t::try_lock() noexcept {
    int expected = 0;
    const bool res = write_lock_.compare_exchange_strong(expected, 1, std::memory_order_relaxed);

    if (res) {
        return {*this, std::adopt_lock};
    }

    BOOST_ASSERT(expected == 1);
    return {};
}

void netlink_write_t::unlock() noexcept {
    const int old_value = write_lock_.fetch_sub(1, std::memory_order_relaxed);
    BOOST_ASSERT(old_value == 1);
}

} // namespace dmn
