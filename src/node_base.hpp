#pragma once

#include "node.hpp"
#include "load_graph.hpp"
#include "impl/packet.hpp"
#include "impl/state_tracker.hpp"

#include <memory>
#include <atomic>
#include <functional>
#include <boost/dll/shared_library.hpp>

namespace dmn {

class stream_t;

class node_base_t: public node_t {
    DMN_PINNED(node_base_t);

public:
    const graph_t config;
    const boost::graph_traits<graph_t>::vertex_descriptor this_node_descriptor;
    const vertex_t& this_node;
    const std::uint16_t host_id_;

    const boost::dll::shared_library lib;

    using callback_t = std::function<void(stream_t&)>;
    callback_t callback_{};

    // Functions:
    node_base_t(boost::asio::io_context& ios, graph_t in, const char* node_id, std::uint16_t host_id);

    std::uint16_t edge_id_for_receiver(std::uint16_t out_edge_index = 0);
    std::uint16_t count_in_edges() const noexcept;
    std::uint16_t count_in_edges_for_receiver(std::uint16_t out_edge_index) const noexcept;
    std::uint16_t count_out_edges() const noexcept;

    virtual void on_packet_accept(packet_t packet) = 0;
    packet_t call_callback(packet_t packet);
    virtual void single_threaded_io_detach() noexcept = 0;
    virtual ~node_base_t() noexcept;
};

std::unique_ptr<node_base_t> make_node(boost::asio::io_context& ios, const std::string& in, const char* node_id, std::uint16_t host_id);

}
