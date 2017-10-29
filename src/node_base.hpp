#pragma once

#include "node.hpp"
#include "load_graph.hpp"
#include "packet.hpp"

#include <boost/dll/shared_library.hpp>

namespace dmn {

struct stream_t;

class node_base_t: public node_t {
protected:
    ~node_base_t() noexcept;

public:
    const graph_t config;
    const boost::graph_traits<graph_t>::vertex_descriptor this_node_descriptor;
    const vertex_t& this_node;

    const boost::dll::shared_library lib;

    using callback_t = void(stream_t&);
    const callback_t callback_;

    // Functions:
    node_base_t(std::istream& in, const char* node_id);

    virtual void start() = 0;
    virtual void on_packet_accept(packet_native_t&& packet) = 0;
    virtual void on_packet_send(packet_native_t&& packet) = 0;
};

void run_node(std::istream& in, const char* node_id);

}
