#pragma once

#include "impl/net/slab_allocator.hpp"

#include <array>
#include <atomic>
#include <mutex>                        // unique_lock
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/optional/optional.hpp>

namespace dmn {

class tcp_write_proto_t {
    DMN_PINNED(tcp_write_proto_t);
public:
    struct reconnect_error_tag{};
    struct send_error_tag{};
    using guard_t = std::unique_lock<tcp_write_proto_t>;

private:
    boost::optional<boost::asio::ip::tcp::socket> socket_;
    using on_send_error_t = std::function<void(boost::system::error_code, guard_t, send_error_tag)>;
    const on_send_error_t on_send_error_;

    using on_reconnect_error_t = std::function<void(boost::system::error_code, guard_t, reconnect_error_tag)>;
    const on_reconnect_error_t on_reconnect_error_;

    using on_operation_finished_t = std::function<void(guard_t )>;
    const on_operation_finished_t on_operation_finished_;

    const boost::asio::ip::tcp::endpoint remote_ep_;
    const std::size_t                    helper_id_;
    std::atomic<int> write_lock_ {0};

    slab_allocator_t slab_;

protected:
    tcp_write_proto_t(
        const char* addr,
        unsigned short port,
        boost::asio::io_service& ios,
        on_send_error_t on_send_error,
        on_operation_finished_t on_operation_finished,
        on_reconnect_error_t on_reconnect_error,
        std::size_t helper_id = 0
    );
    ~tcp_write_proto_t();

public:
    void async_reconnect(guard_t g);

    std::size_t helper_id() const noexcept {
        return helper_id_;
    }

    void async_send(guard_t g, std::array<boost::asio::const_buffer, 2> data);

    void async_send(guard_t g, boost::asio::const_buffers_1 data) {
        const std::array<boost::asio::const_buffer, 2> buf{*data.begin(), boost::asio::const_buffer{}};
        async_send(std::move(g), buf);
    }

    // Closes the socket
    void close() noexcept;

    guard_t try_lock() noexcept;
    void unlock() noexcept;
};

} // namespace dmn
