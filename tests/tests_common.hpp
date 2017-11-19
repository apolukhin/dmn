#pragma once

#include <boost/asio/io_service.hpp>
#include "node_base.hpp"

namespace tests {
namespace detail {

template <class Node>
void shutdown_nodes_impl(Node&& node) {
    node->shutdown_gracefully();
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
    node.reset();
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
}

template <class Node, class... Nodes>
void shutdown_nodes_impl(Node&& node, Nodes&&... nodes) {
    shutdown_nodes_impl(node);
    detail::shutdown_nodes_impl(std::forward<Nodes>(nodes)...);
}

} // namespace detail



template <class Node, class... Nodes>
void shutdown_nodes(Node&& node, Nodes&&... nodes) {
    // First node must be shut down by app logic
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
    node.reset();
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
    detail::shutdown_nodes_impl(std::forward<Nodes>(nodes)...);
}

inline dmn::packet_native_t clone(const dmn::packet_native_t& orig) {
    dmn::packet_native_t cloned{
        dmn::packet_header_native_t{orig.header_},
        {}
    };

    cloned.body_.data_ = orig.body_.data_;
    return cloned;
}

inline dmn::packet_body_native_t clone(const dmn::packet_body_native_t& orig) {
    dmn::packet_body_native_t cloned{};
    cloned.data_ = orig.data_;
    return cloned;
}

} // namespace tests
