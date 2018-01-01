#pragma once

#include <boost/asio/io_service.hpp>
#include "node_base.hpp"

namespace tests {
namespace detail {

template <class Node>
void shutdown_nodes_impl(Node&& node) {
    BOOST_TEST(!!node);

    node->shutdown_gracefully();
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
    node.reset();
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
}

template <class Node, class... Nodes>
void shutdown_nodes_impl(Node&& node, Nodes&&... nodes) {
    BOOST_TEST(!!node);
    shutdown_nodes_impl(node);
    detail::shutdown_nodes_impl(std::forward<Nodes>(nodes)...);
}

} // namespace detail



template <class Node, class... Nodes>
void shutdown_nodes(Node&& node, Nodes&&... nodes) {
    BOOST_TEST(!!node);

    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
    node.reset();
    dmn::node_base_t::ios().reset();
    dmn::node_base_t::ios().poll();
    detail::shutdown_nodes_impl(std::forward<Nodes>(nodes)...);
}

inline dmn::packet_t clone(const dmn::packet_t& orig) {
    dmn::packet_t cloned{
        dmn::packet_storage_t{orig.raw_storage()}
    };
    return cloned;
}

} // namespace tests
