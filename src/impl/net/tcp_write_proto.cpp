#include "impl/net/tcp_write_proto.hpp"

#include "impl/net/proto_common.hpp"
#include "impl/net/wrap_handler.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

namespace dmn {

#define ASSERT_GUARD(g) \
    BOOST_ASSERT_MSG(g, "Attempt to connect without holding a netlink guard");  \
    BOOST_ASSERT_MSG(g.mutex() == this, "Sending with foreign netlink guard");  \
    /**/


tcp_write_proto_t::tcp_write_proto_t(
    const char* addr,
    unsigned short port,
    boost::asio::io_service& ios,
    on_send_error_t on_send_error,
    on_operation_finished_t on_operation_finished,
    on_reconnect_error_t on_reconnect_error,
    std::size_t helper_id
)
    : socket_(ios)
    , on_send_error_(std::move(on_send_error))
    , on_reconnect_error_(std::move(on_reconnect_error))
    , on_operation_finished_(std::move(on_operation_finished))
    , remote_ep_{boost::asio::ip::address::from_string(addr), port}
    , helper_id_(helper_id)
{}

void tcp_write_proto_t::async_reconnect(tcp_write_proto_t::guard_t g) {
    ASSERT_GUARD(g);
    auto on_connect = [guard = std::move(g), this](const boost::system::error_code& e) mutable {
        if (e) {
            on_reconnect_error_(e, std::move(guard), {});
            return;
        }

        dmn::set_socket_options(socket_);
        dmn::set_writing_ack_timeout(socket_);
        on_operation_finished_(std::move(guard));
    };

    socket_.async_connect(
        remote_ep_,
        make_slab_alloc_handler(slab_, std::move(on_connect))
    );
}



void tcp_write_proto_t::async_send(guard_t g, std::array<boost::asio::const_buffer, 2> buf) {
    ASSERT_GUARD(g);
    BOOST_ASSERT_MSG(socket_.is_open(), "We allow closing write connections only by destructor calls.");

    auto on_write = [guard = std::move(g), buf, this](const boost::system::error_code& e, std::size_t bytes_written) mutable {
        if (e) {
            on_send_error_(e, std::move(guard), {});
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

    
tcp_write_proto_t::~tcp_write_proto_t() noexcept {
    boost::system::error_code ignore;
    socket_.shutdown(decltype(socket_)::shutdown_both, ignore);
    socket_.close(ignore);
    BOOST_ASSERT_MSG(write_lock_.load() == 0, "The write connection is locked in destructor");
}

tcp_write_proto_t::guard_t tcp_write_proto_t::try_lock() noexcept {
    int expected = 0;
    const bool res = write_lock_.compare_exchange_strong(expected, 1, std::memory_order_relaxed);

    if (res) {
        BOOST_ASSERT_MSG(expected == 0, "Lock is broken, allowed values are 0 and 1");
        return {*this, std::adopt_lock};
    }
    BOOST_ASSERT_MSG(expected == 1, "Lock is broken, allowed values are 0 and 1");

    return {};
}

void tcp_write_proto_t::unlock() noexcept {
    const int old_value = write_lock_.fetch_sub(1, std::memory_order_relaxed);
    BOOST_ASSERT_MSG(old_value != 0, "Lock underflow");
    BOOST_ASSERT_MSG(old_value == 1, "Locked multiple times");
}

} // namespace dmn
