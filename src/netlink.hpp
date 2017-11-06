#pragma once

#include "node_base.hpp"
#include "write_work.hpp"

#include "packet.hpp"

#include <atomic>
#include <mutex>
#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace dmn {

class netlink_read_t;
class netlink_write_t;
using netlink_read_ptr = std::unique_ptr<netlink_read_t>;
using netlink_write_ptr = std::unique_ptr<netlink_write_t>;

class netlink_read_t {
private:
    boost::asio::ip::tcp::socket socket_;
    node_base_t& node_;

    std::aligned_storage_t<sizeof(packet_native_t), alignof(packet_native_t)> network_in_holder_;
    packet_native_t  native_in_;
    read_ticket_t ticket_;

    netlink_read_t(netlink_read_t&&) = delete;
    netlink_read_t(const netlink_read_t&) = delete;
    netlink_read_t& operator=(netlink_read_t&&) = delete;
    netlink_read_t& operator=(const netlink_read_t&) = delete;

    void process_and_async_read();
    void async_read_data();
    void async_read();

    netlink_read_t(node_base_t& node, boost::asio::ip::tcp::socket&& socket, read_ticket_t&& ticket);

public:
    static netlink_read_ptr construct(node_base_t& node, boost::asio::ip::tcp::socket&& socket, read_ticket_t&& ticket) {
        return netlink_read_ptr{new netlink_read_t(
            node, std::move(socket), std::move(ticket)
        )};
    }


    ~netlink_read_t();
};

class netlink_write_t {
public:
    using guard_t = std::unique_lock<netlink_write_t>;

private:
    boost::asio::ip::tcp::socket socket_;
    node_base_t& node_;
    const boost::asio::ip::tcp::endpoint remote_ep_;
    std::aligned_storage_t<sizeof(packet_native_t), alignof(packet_native_t)> network_out_holder_;

    std::atomic<int> write_lock_ {0};

    netlink_write_t(netlink_write_t&&) = delete;
    netlink_write_t(const netlink_write_t&) = delete;
    netlink_write_t& operator=(netlink_write_t&&) = delete;
    netlink_write_t& operator=(const netlink_write_t&) = delete;

    void async_connect(guard_t&& g);

    netlink_write_t(node_base_t& node, const char* addr, unsigned short port);

public:
    static netlink_write_ptr construct(node_base_t& node, const char* addr, unsigned short port) {
        return netlink_write_ptr{new netlink_write_t(
            node, addr, port
        )};
    }

    void async_send(write_ticket_t&& ticket, guard_t&& g, packet_native_t&& data);

    guard_t try_lock() noexcept;
    void unlock() noexcept;

    ~netlink_write_t();
};

} // namespace dmn
