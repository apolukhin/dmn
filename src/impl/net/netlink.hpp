#pragma once

#include "utility.hpp"
#include <memory>

namespace dmn {

// Structure that is pined to memory location and combines protocol with buffer/storage.
template <class Packet, class Proto>
struct netlink_t: Proto {
    Packet packet;

private:
    DMN_PINNED(netlink_t);

public:
    template <class... Args>
    explicit netlink_t(Args&&... args)
        : Proto(std::forward<Args>(args)...)
    {}

    template <class... Args>
    static std::unique_ptr<netlink_t> construct(Args&&... args) {
        return std::unique_ptr<netlink_t>(
            new netlink_t{std::forward<Args>(args)...}
        );
    }


    static netlink_t& to_link(Proto& link) noexcept {
        return static_cast<netlink_t&>(link);
    }
    static const netlink_t& to_link(const Proto& link) noexcept {
        return static_cast<const netlink_t&>(link);
    }
};



}
