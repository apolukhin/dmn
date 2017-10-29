#pragma once

#include "node_base.hpp"

#include "packet.hpp"

#include <atomic>
#include <mutex>
#include <boost/make_unique.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace dmn {

class netlink_t;
using netlink_ptr = std::unique_ptr<netlink_t>;

class netlink_t {
    boost::asio::ip::tcp::socket socket_;
    node_base_t& node_;

    std::aligned_storage_t<sizeof(packet_native_t), alignof(packet_native_t)> network_in_holder_;
    std::aligned_storage_t<sizeof(packet_native_t), alignof(packet_native_t)> network_out_holder_;
    packet_native_t  native_in_;

    std::atomic<int> write_lock_ {0};

    netlink_t(netlink_t&&) = delete;
    netlink_t(const netlink_t&) = delete;
    netlink_t& operator=(netlink_t&&) = delete;
    netlink_t& operator=(const netlink_t&) = delete;

    void process_and_async_read();
    void async_read_data();
    void async_read();

    netlink_t(node_base_t& node, const char* addr);
    netlink_t(node_base_t& node, boost::asio::ip::tcp::socket&& socket);

public:
    using guart_t = std::unique_lock<netlink_t>;

    template <class... Args>
    static netlink_ptr construct(Args&&... args) {
        return netlink_ptr{new netlink_t(
            std::forward<Args>(args)...
        )};
    }

    void async_send(guart_t&& g, packet_native_t&& data);

    guart_t try_lock() noexcept;
    void unlock() noexcept;

    ~netlink_t();
};

} // namespace dmn
