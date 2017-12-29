#pragma once

#include "impl/pinned_container.hpp"
#include "impl/silent_mt_queue.hpp"
#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/net/tcp_write_proto.hpp"

namespace dmn {

template <class Packet>
struct edge_out_t {
    using link_t = netlink_t<Packet, tcp_write_proto_t>;
    using netlinks_t = pinned_container<link_t>;
    using queue_t = silent_mt_queue<Packet>;

private:
    netlinks_t  netlinks_; // `const` after set_links()
    queue_t     data_to_send_;

    template <class Link>
    static boost::asio::const_buffers_1 get_buf(netlink_t<packet_network_t, Link>& link, packet_network_t v) noexcept {
        BOOST_ASSERT(!empty_packet(v));

        auto p = std::move(v); // workaround for self move
        link.packet = std::move(p);

        return link.packet.const_buffer();
    }

    template <class Link>
    static boost::asio::const_buffers_1 get_buf(netlink_t<boost::asio::const_buffer, Link>& link, boost::asio::const_buffer v) noexcept {
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
    static link_t& link_from_guard(const tcp_write_proto_t::guard_t& guard) noexcept {
        BOOST_ASSERT_MSG(guard, "Empty link guard");
        return static_cast<link_t&>(*guard.mutex());
    }

    void preinit_links(std::size_t count) {
        netlinks_.init(count);
    }

    template <class... Args>
    void inplace_construct_link(std::size_t i, Args&&... args) {
        netlinks_.inplace_construct_link(i, std::forward<Args>(args)...);
    }

    void connect_links() noexcept {
        for (auto& v : netlinks_) {
            v.async_connect(v.try_lock());
        }
    }

    void assert_no_more_data() noexcept {
        BOOST_ASSERT_MSG(!data_to_send_.try_pop(), "Have data to send");
    }

    void close_links() noexcept {
        for (auto& v: netlinks_) {   // TODO: balancing
            tcp_write_proto_t::guard_t lock = v.try_lock();
            if (lock) {
                v.close(std::move(lock));
            } else {
                // Link is already closed // TODO: validate
            }
        }
    }

    void try_send() {
        for (auto& v: netlinks_) {   // TODO: balancing
            tcp_write_proto_t::guard_t lock = v.try_lock();
            if (!lock) {
                continue;
            }

            auto p = data_to_send_.try_pop();
            if (!p) {
                return;
            }

            const auto buf = get_buf(v, std::move(*p));
            v.async_send(std::move(lock), buf);
            break;
        }
    }

    void try_steal_work(tcp_write_proto_t::guard_t&& guard) {
        // Work stealing
        auto v = data_to_send_.try_pop();
        if (!v) {
            return;
        }

        auto& link = link_from_guard(guard);
        const auto buf = get_buf(link, std::move(*v));
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
