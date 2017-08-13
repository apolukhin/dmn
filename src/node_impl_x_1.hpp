#pragma once

#include "node_base.hpp"
#include "stream.hpp"

#include <mutex>

namespace dmn {

class node_impl_x_1: public virtual node_base_t {
    std::mutex                  streams_mutex_;
    std::deque<stream_ptr_t>    streams_;

    stream_ptr_t get_stream() {
        std::lock_guard<std::mutex> l(streams_mutex_);
        auto s = std::move(streams_.front());
        streams_.pop_front();

        return s;
    }

public:
    node_impl_x_1(std::istream& in, const char* node_id)
        : node_base_t(in, node_id)
    {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );

        BOOST_ASSERT(edges_out.second - edges_out.first == 1);
        const vertex_t& out_vertex = config[target(*edges_out.first, config)];

        for (const auto& host : out_vertex.hosts) {
            streams_.emplace_back(connect_stream(*this, host.c_str()));
        }
    }

    void send_message(message_t&& m) {
        send_data(
            get_stream(),
            std::move(m)
        );

    }

    ~node_impl_x_1() noexcept = default;
};

}
