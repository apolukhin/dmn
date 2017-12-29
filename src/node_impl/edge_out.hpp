#pragma once

#include "netlink.hpp"
#include "silent_mt_queue.hpp"
#include "netlink_with_packet.hpp"
#include "packet_network.hpp"

#include <vector>

namespace dmn {

template <class Packet>
struct edge_out_t {
    using netlinks_t = std::vector<netlink_with_packet_t<Packet>>;
    using queue_t = silent_mt_queue<Packet>;

private:
    netlinks_t  netlinks_; // `const` after set_links()
    queue_t     data_to_send_;

    static boost::asio::const_buffers_1 get_buf(netlink_with_packet_t<packet_network_t>& link, packet_network_t v) noexcept {
        BOOST_ASSERT(!empty_packet(v));

        auto p = std::move(v); // workaround for self move
        link.packet = std::move(p);

        return link.packet.const_buffer();
    }

    static boost::asio::const_buffers_1 get_buf(netlink_with_packet_t<boost::asio::const_buffer>& link, boost::asio::const_buffer v) noexcept {
        BOOST_ASSERT(!empty_packet(v));

        link.packet = std::move(v);
        return boost::asio::const_buffers_1{link.packet};
    }

    static bool empty_packet(boost::asio::const_buffer buf) noexcept {
        return boost::asio::buffer_size(buf) == 0;
    }

    static bool empty_packet(const packet_network_t& p) noexcept {
        return p.empty();
    }

public:
    void set_links(netlinks_t links) noexcept {
        BOOST_ASSERT_MSG(netlinks_.empty(), "Mutiple calls to set_links are not allowed: netlinks_ must not be modified after starting to connect");
        std::sort(links.begin(), links.end());
        netlinks_ = std::move(links);

        for (auto& v : netlinks_) {
            v.link->async_connect(v.link->try_lock());
        }
    }

    void close_links(bool forced = false) noexcept {
        BOOST_ASSERT_MSG(forced || !data_to_send_.try_pop(), "have data to send during soft shutdown");
        for (auto& v: netlinks_) {   // TODO: balancing
            netlink_write_t::guard_t lock = v.link->try_lock();
            if (lock) {
                v.link->async_close(std::move(lock));
            } else {
                // Link is already closed // TODO: validate
            }
        }
    }

    auto find_netlink(const netlink_write_t::guard_t& guard) noexcept {
        BOOST_ASSERT_MSG(guard, "Empty netlink guard");
        netlink_write_t& link = *guard.mutex();
        const auto pair = std::equal_range(netlinks_.begin(), netlinks_.end(), &link);
        BOOST_ASSERT_MSG(pair.second - pair.first != 0, "Netlink not found in the vector of known netlinks");
        return pair.first;
    }

    void try_send() {
        for (auto& v: netlinks_) {   // TODO: balancing
            netlink_write_t::guard_t lock = v.link->try_lock();
            if (!lock) {
                continue;
            }

            auto p = data_to_send_.try_pop();
            if (!p) {
                return;
            }

            auto* connection = v.link.get();
            const auto buf = get_buf(v, std::move(*p));
            connection->async_send(std::move(lock), buf);
            break;
        }
    }

    void try_steal_work(netlink_write_t::guard_t&& guard) {
        BOOST_ASSERT_MSG(guard, "Empty netlink guard");
        netlink_write_t& link = *guard.mutex();
        // Work stealing
        auto v = data_to_send_.try_pop();
        if (!v) {
            return;
        }

        auto it = find_netlink(guard);
        const auto buf = get_buf(*it, std::move(*v));
        link.async_send(std::move(guard), buf);
    }

    void push_immediate(Packet p) {
        if (!empty_packet(p)) {
            data_to_send_.silent_push_front(std::move(p));
        }
    }
    void push(Packet p) {
        BOOST_ASSERT_MSG(!empty_packet(p), "Scheduling an empy packet without headers for sending. This must not be produced by accepting vertexes");
        data_to_send_.silent_push(std::move(p));
    }
};


}
