#pragma once

#include "node.hpp"
#include "load_graph.hpp"
#include "impl/packet.hpp"
#include "impl/state_tracker.hpp"

#include <memory>
#include <atomic>
#include <boost/dll/shared_library.hpp>

namespace dmn {

struct stream_t;

class node_base_t: public node_t, public state_tracker_t {
    DMN_PINNED(node_base_t);

public:
    const graph_t config;
    const boost::graph_traits<graph_t>::vertex_descriptor this_node_descriptor;
    const vertex_t& this_node;
    const std::uint16_t host_id_;

    const boost::dll::shared_library lib;

    using callback_t = void(*)(stream_t&);
    callback_t callback_ = nullptr;

    // Functions:
    node_base_t(const graph_t& in, const char* node_id, std::uint16_t host_id);

    virtual void on_packet_accept(packet_t packet) = 0;
    packet_t call_callback(packet_t packet) noexcept;
    virtual ~node_base_t() noexcept;
};

std::unique_ptr<node_base_t> make_node(std::istream& in, const char* node_id, std::uint16_t host_id);

}
