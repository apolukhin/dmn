#include "netlink.hpp"

#include "packet_network.hpp"
#include "wrap_handler.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

namespace dmn {

#define ASSERT_GUARD(g) \
    BOOST_ASSERT_MSG(g, "Attempt to connect without holding a netlink guard");  \
    BOOST_ASSERT_MSG(g.mutex() == this, "Sending with foreign netlink guard");  \
    /**/


namespace {
    inline packet_network_t& netpacket(std::aligned_storage_t<sizeof(packet_t), alignof(packet_t)>& data) noexcept {
        static_assert(sizeof(packet_t) == sizeof(packet_network_t), "");
        static_assert(alignof(packet_t) == alignof(packet_network_t), "");

        return *reinterpret_cast<packet_network_t*>(
            &data
        );
    }

    inline void netpacket_replace(std::aligned_storage_t<sizeof(packet_t), alignof(packet_t)>& data, packet_network_t val) noexcept {
        netpacket(data).~packet_network_t();
        new (&data) packet_network_t(std::move(val));
    }


    template <class Socket>
    void set_socket_options(Socket& s) {
        boost::asio::ip::tcp::no_delay option(true);
        boost::system::error_code ignore;
        s.set_option(option, ignore);
    }
}

void netlink_read_t::process_and_async_read() {
    packet_t data { std::move(netpacket(network_in_holder_)).to_native() };
    async_read();

    on_data_(std::move(data));
}

void netlink_read_t::async_read_data() {
    if (!netpacket(network_in_holder_).body_size()) {
        process_and_async_read();
        return;
    }

    const auto buf = netpacket(network_in_holder_).body_mutable_buffer();
    boost::asio::async_read(
        socket_,
        buf,
        make_slab_alloc_handler(
            slab_,
            [this](const boost::system::error_code& e, std::size_t bytes_read) {
                if (e) {
                    process_error(e);
                    return;
                }
                BOOST_ASSERT_MSG(bytes_read == netpacket(network_in_holder_).body_size(), "Wrong bytes read for data");

                process_and_async_read();
            }
        )
    );
}


netlink_read_t::netlink_read_t(boost::asio::ip::tcp::socket&& socket, on_error_t on_error, on_data_t on_data)
    : socket_(std::move(socket))
    , on_error_(std::move(on_error))
    , on_data_(std::move(on_data))
{
    set_socket_options(socket_);
    new (&network_in_holder_) packet_network_t();
    async_read();
}

netlink_write_t::netlink_write_t(const char* addr, unsigned short port, boost::asio::io_service& ios, on_error_t on_error, on_operation_finished_t on_operation_finished, std::size_t helper_id)
    : socket_(ios)
    , on_error_(std::move(on_error))
    , on_operation_finished_(std::move(on_operation_finished))
    , remote_ep_{boost::asio::ip::address::from_string(addr), port}
    , helper_id_(helper_id)
{}

netlink_read_t::~netlink_read_t() {
    netpacket(network_in_holder_).~packet_network_t();
}

netlink_write_t::~netlink_write_t() = default;

void netlink_write_t::async_connect(netlink_write_t::guard_t&& g) {
    ASSERT_GUARD(g);
    auto on_connect = [guard = std::move(g), this](const boost::system::error_code& e) mutable {
        if (e) {
            process_error(e, std::move(guard));
            return;
        }

        set_socket_options(socket_);
        on_operation_finished_(std::move(guard));
    };

    socket_.async_connect(
        remote_ep_,
        make_slab_alloc_handler(slab_, std::move(on_connect))
    );
}

void netlink_read_t::async_read() {
    BOOST_ASSERT(socket_.is_open());

    const auto buf = netpacket(network_in_holder_).header_mutable_buffer();
    auto on_read = [this](const boost::system::error_code& e, std::size_t bytes_read) {
        if (e) {
            process_error(e);
            return;
        }

        BOOST_ASSERT_MSG(bytes_read == sizeof(packet_header_t), "Wrong bytes read for header");

        switch (netpacket(network_in_holder_).packet_type()) {
        case packet_types_enum::DATA:
            async_read_data();
            return;
        case packet_types_enum::SHUTDOWN_GRACEFULLY: {
            boost::system::error_code ignore;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
            return;
        }
        default:
            BOOST_ASSERT_MSG(false, "Unknown packet type"); // TODO: log error, not assert
            return;
        };
    };

    boost::asio::async_read(
        socket_,
        buf,
        make_slab_alloc_handler(slab_, std::move(on_read))
    );
}

void netlink_write_t::async_send(guard_t&& g, boost::asio::const_buffers_1 buf) {
    ASSERT_GUARD(g);
    BOOST_ASSERT(socket_.is_open());

    auto on_write = [guard = std::move(g), buf, this](const boost::system::error_code& e, std::size_t bytes_written) mutable {
        if (e) {
            process_error(e, std::move(guard));
            return;
        }
        BOOST_ASSERT_MSG(boost::asio::buffer_size(buf) == bytes_written, "Wrong bytes count written");
        on_operation_finished_(std::move(guard));
    };

    boost::asio::async_write(
        socket_,
        buf,
        make_slab_alloc_handler(slab_, std::move(on_write))
    );
}

void netlink_write_t::async_close(guard_t&& g) noexcept {
    ASSERT_GUARD(g);
    boost::system::error_code ignore;
    socket_.shutdown(decltype(socket_)::shutdown_both, ignore);
    socket_.close(ignore);
    g.release(); // leaving link in locked state
    BOOST_ASSERT_MSG(write_lock_.load(std::memory_order_relaxed) != 0, "After close the connection is not locked");
    BOOST_ASSERT_MSG(write_lock_.load(std::memory_order_relaxed) == 1, "After close the connection is locked multiple times");
}


netlink_write_t::guard_t netlink_write_t::try_lock() noexcept {
    int expected = 0;
    const bool res = write_lock_.compare_exchange_strong(expected, 1, std::memory_order_relaxed);

    if (res) {
        BOOST_ASSERT_MSG(expected == 0, "Lock is broken, allowed values are 0 and 1");
        return {*this, std::adopt_lock};
    }
    BOOST_ASSERT_MSG(expected == 1, "Lock is broken, allowed values are 0 and 1");

    return {};
}

void netlink_write_t::unlock() noexcept {
    const int old_value = write_lock_.fetch_sub(1, std::memory_order_relaxed);
    BOOST_ASSERT_MSG(old_value != 0, "Lock underflow");
    BOOST_ASSERT_MSG(old_value == 1, "Locked multiple times");
}

} // namespace dmn
