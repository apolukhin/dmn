#pragma once

#include "node.hpp"
#include "load_graph.hpp"
#include "packet.hpp"
#include "write_work.hpp"

#include <memory>
#include <atomic>
#include <boost/dll/shared_library.hpp>

namespace dmn {

struct stream_t;

enum class stop_enum: unsigned {
    RUN = 0,
    STOPPING_READ = 1,
    STOPPING_WRITE = 2,
    STOPPED = 3,
};

class node_base_t: public node_t {
public:
    const graph_t config;
    const boost::graph_traits<graph_t>::vertex_descriptor this_node_descriptor;
    const vertex_t& this_node;

    const boost::dll::shared_library lib;

    using callback_t = void(*)(stream_t&);
    callback_t callback_ = nullptr;

    // Functions:
    node_base_t(const graph_t& in, const char* node_id);

    std::atomic<stop_enum>  state_ {stop_enum::RUN};
    read_counter_t          read_counter_{};
    write_counter_t         write_counter_{};

    virtual void stop_reading() = 0;
    virtual void stop_writing() = 0;

    stop_enum on_packet_accept(write_ticket_t&& write_ticket, packet_native_t&& packet);
    virtual void on_packet_send(write_ticket_t&& g, packet_native_t&& packet) = 0;
    virtual ~node_base_t() noexcept;
};

std::unique_ptr<node_base_t> make_node(std::istream& in, const char* node_id);

}
