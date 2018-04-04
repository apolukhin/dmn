#include "nodes_tester.hpp"

#include <boost/asio/io_service.hpp>
#include <thread>

namespace tests {

void nodes_tester_t::poll_impl() {
    constexpr auto ios_poll = []() {
        dmn::node_base_t::ios().reset();
        dmn::node_base_t::ios().poll();
    };

    threads_.reserve(threads_count_);

    for (int i = 1; i < threads_count_; ++i) {
        threads_.emplace_back(ios_poll);
    }
    ios_poll();
    for (auto& t: threads_) {
        t.join();
    }
    threads_.clear();
}

nodes_tester_t::nodes_tester_t(const std::string& links, std::initializer_list<node_params> params) {
    nodes_.reserve(params.size());

    unsigned short port_num = 44000;
    std::string g = "digraph test {\n";
    for (const auto& p : params) {
        g += p.node_name;
        g += " [hosts = \"";
        for (unsigned i = 0; i < p.hosts; ++i) {
            if (i) {
                g += ';';
            }
            g += "127.0.0.1:" + std::to_string(port_num);
            ++ port_num;
        }
        g += "\"]\n";
    }
    g += links + "}";

    for (const auto& p : params) {
        for (unsigned host_id = 0; host_id < p.hosts; ++host_id) {
            auto new_node = dmn::make_node(g, p.node_name, host_id);
            BOOST_TEST(!!new_node);

            nodes_.push_back(std::move(new_node));
            nodes_.back()->callback_ = p.callback;
        }
    }
}

void nodes_tester_t::poll() {
    poll_impl();

    for (auto& node : nodes_) {
        node->shutdown_gracefully();
        poll_impl();

        node.reset();
        poll_impl();
    }
    nodes_.clear();
}

}
