#pragma once

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/net/tcp_read_proto.hpp"
#include "impl/compare_addrs.hpp"

#include <deque>
#include <memory>
#include <mutex>

namespace dmn {

template <class Packet>
struct edge_in_t {
    using link_t = netlink_t<Packet, tcp_read_proto_t>;

private:
    std::mutex                          netlinks_mutex_;
    std::deque<
        std::unique_ptr<link_t>
    >   netlinks_;

public:
    std::size_t close_links() noexcept {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        for (auto& v: netlinks_) {   // TODO: balancing
            v->close();
        }

        return netlinks_.size();
    }

    link_t& add_link(std::unique_ptr<link_t> link_ptr) {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        auto it = std::lower_bound(netlinks_.begin(), netlinks_.end(), link_ptr);
        return **netlinks_.insert(it, std::move(link_ptr));
    }

    std::size_t remove_link(link_t& link) {
        link.close();
        link.packet.clear();

        std::unique_lock<std::mutex> guard(netlinks_mutex_);

        const auto pair = std::equal_range(netlinks_.begin(), netlinks_.end(), link, compare_addrs_t{});
        BOOST_ASSERT_MSG(pair.second - pair.first != 0, "Netlink not found in the vector of known netlinks");
        netlinks_.erase(pair.first);

        return netlinks_.size();
    }
};


}
