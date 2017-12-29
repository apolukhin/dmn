#pragma once

#include "packet.hpp"
#include "slab_allocator.hpp"

#include <atomic>
#include <mutex>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

namespace dmn {

class netlink_read_t;
class netlink_write_t;
using netlink_read_ptr = std::unique_ptr<netlink_read_t>;
using netlink_write_ptr = std::unique_ptr<netlink_write_t>;

class netlink_read_t {
private:
    boost::asio::ip::tcp::socket socket_;
    using on_error_t = std::function<void(netlink_read_t*, const boost::system::error_code&)>;
    const on_error_t on_error_;

    using on_data_t = std::function<void(packet_t&&)>;
    const on_data_t on_data_;

    std::aligned_storage_t<sizeof(packet_t), alignof(packet_t)> network_in_holder_;

    slab_allocator_t slab_;

    netlink_read_t(netlink_read_t&&) = delete;
    netlink_read_t(const netlink_read_t&) = delete;
    netlink_read_t& operator=(netlink_read_t&&) = delete;
    netlink_read_t& operator=(const netlink_read_t&) = delete;

    void process_and_async_read();
    void async_read_data();
    void async_read();
    void process_error(const boost::system::error_code& e) {
        on_error_(this, e);
    }

    netlink_read_t(boost::asio::ip::tcp::socket&& socket, on_error_t on_error, on_data_t on_data);

public:
    template <class OnError, class OnData>
    static netlink_read_ptr construct(boost::asio::ip::tcp::socket&& socket, OnError&& on_error, OnData&& on_data) {
        return netlink_read_ptr{new netlink_read_t(
            std::move(socket), std::forward<OnError>(on_error), std::forward<OnData>(on_data)
        )};
    }

    void cancel() noexcept {
        socket_.cancel();
    }

    ~netlink_read_t();
};

class netlink_write_t {
public:
    using guard_t = std::unique_lock<netlink_write_t>;

private:
    boost::asio::ip::tcp::socket socket_;
    using on_error_t = std::function<void(const boost::system::error_code&, guard_t&&)>;
    const on_error_t on_error_;

    using on_operation_finished_t = std::function<void(guard_t&&)>;
    const on_operation_finished_t on_operation_finished_;

    const boost::asio::ip::tcp::endpoint remote_ep_;
    const std::size_t                    helper_id_;
    std::atomic<int> write_lock_ {0};


    slab_allocator_t slab_;

    netlink_write_t(netlink_write_t&&) = delete;
    netlink_write_t(const netlink_write_t&) = delete;
    netlink_write_t& operator=(netlink_write_t&&) = delete;
    netlink_write_t& operator=(const netlink_write_t&) = delete;

    void process_error(const boost::system::error_code& e, guard_t&& g) {
        on_error_(e, std::move(g));
    }

    netlink_write_t(const char* addr, unsigned short port, boost::asio::io_service& ios, on_error_t on_error, on_operation_finished_t on_operation_finished, std::size_t helper_id);

public:
    void async_connect(guard_t&& g);

    template <class OnError, class OnFinish>
    static netlink_write_ptr construct(const char* addr, unsigned short port, boost::asio::io_service& ios, OnError&& on_error, OnFinish&& on_operation_finished, std::size_t helper_id = 0) {
        return netlink_write_ptr{new netlink_write_t(
            addr, port, ios, std::forward<OnError>(on_error), std::forward<OnFinish>(on_operation_finished), helper_id
        )};
    }

    std::size_t helper_id() const noexcept {
        return helper_id_;
    }

    void async_send(guard_t&& g, boost::asio::const_buffers_1 data);

    // Closes the socket and leaves the link in locked state
    void async_close(guard_t&& g) noexcept;
    void async_cancel() noexcept {
        socket_.cancel();
    }

    guard_t try_lock() noexcept;
    void unlock() noexcept;

    ~netlink_write_t();
};

} // namespace dmn
