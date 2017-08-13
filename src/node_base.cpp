#include "node_base.hpp"

#include "load_graph.hpp"
#include <istream>

#include "node_impl_0_x.hpp"
#include "node_impl_1_x.hpp"
#include "node_impl_x_0.hpp"
#include "node_impl_x_1.hpp"

namespace dmn {

namespace {
    auto get_this_node_descriptor(const graph_t& g, const char* node_id) {
        BOOST_ASSERT(node_id);
        const auto vds = vertices(g);
        const auto it_descriptor = std::find_if(vds.first, vds.second, [node_id, &g](auto v_descriptor){
            return g[v_descriptor].node_id == node_id;
        });
        if (it_descriptor == vds.second) {
            throw std::runtime_error("Node with id '" + std::string(node_id) + "' does not exist in graph");
        }

        return *it_descriptor;
    }
}

node_base_t::node_base_t(std::istream& in, const char* node_id)
    : config(load_graph(in))
    , this_node_descriptor(get_this_node_descriptor(config, node_id))
    , this_node(config[this_node_descriptor])
{}

node_base_t::~node_base_t() noexcept = default;


void run_node(std::istream& in, const char* node_id) {

}

}
