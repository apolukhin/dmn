#pragma once

#include "node.hpp"
#include "load_graph.hpp"

#include <boost/dll/shared_library.hpp>

namespace dmn {

struct message_t;

class node_base_t: public node_t {
protected:
    ~node_base_t() noexcept;

public:
    const graph_t config;
    const boost::graph_traits<graph_t>::vertex_descriptor this_node_descriptor;
    const vertex_t& this_node;

    const boost::dll::shared_library lib;

    using callback_t = void(const dmn::node_t&, message_t&);
    const callback_t callback_;

    // Functions:
    node_base_t(std::istream& in, const char* node_id);

    virtual void on_message(message_t&& /*m*/) = 0;

    void add_message(const void* data, std::size_t size, const char* type = "") override {
        throw std::logic_error("You shall not construct messages on non source node");
    }

    void send_message() override {
        throw std::logic_error("You shall not send messages on sink node");
    }
};

void run_node(std::istream& in, const char* node_id);

}
