#pragma once

#include "netlink.hpp"

namespace dmn {

template <class Packet>
struct netlink_with_packet_t {
    netlink_write_ptr link;
    Packet packet;
};

template <class Packet>
bool operator<(const netlink_with_packet_t<Packet>& lhs, const netlink_write_t* rhs) noexcept {
    BOOST_ASSERT_MSG(rhs, "Nullptr instead of a netlink");
    return lhs.link.get() < rhs;
}

template <class Packet>
bool operator<(const netlink_with_packet_t<Packet>& lhs, const netlink_with_packet_t<Packet>& rhs) noexcept {
    return lhs.link.get() < rhs.link.get();
}

template <class Packet>
bool operator<(const netlink_write_t* lhs, const netlink_with_packet_t<Packet>& rhs) noexcept {
    BOOST_ASSERT_MSG(lhs, "Nullptr instead of a netlink");
    return lhs < rhs.link.get();
}

}
