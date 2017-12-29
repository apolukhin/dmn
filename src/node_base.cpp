#include "node_base.hpp"

#include "load_graph.hpp"
#include <istream>

#include "impl/node_parts/read_0.hpp"
#include "impl/node_parts/read_1.hpp"
#include "impl/node_parts/write_0.hpp"
#include "impl/node_parts/write_1.hpp"
#include "impl/node_parts/write_n.hpp"

#include <boost/make_unique.hpp>

namespace dmn {

namespace {
    auto get_this_node_descriptor(const graph_t& g, const char* node_id) {
        BOOST_ASSERT_MSG(node_id, "Searching for node without ID. Error in load_graph function or in make_node");
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

node_base_t::node_base_t(const graph_t& in, const char* node_id, std::uint16_t host_id)
    : config(in)
    , this_node_descriptor(get_this_node_descriptor(config, node_id))
    , this_node(config[this_node_descriptor])
    , host_id_(host_id)
{}

packet_t node_base_t::call_callback(packet_t packet) noexcept {
    stream_t s{*this, std::move(packet)};
    node_base_t::callback_(s);
    return s.move_out_data();
}

node_base_t::~node_base_t() noexcept = default;





template <class Read, class Write>
struct node_in_x_out_x final: Read, Write {
    node_in_x_out_x(const graph_t& in, const char* node_id, std::uint16_t host_id)
        : node_base_t(in, node_id, host_id)
        , Read()
        , Write()
    {}

    using Read::on_stop_reading;
    using Write::on_packet_accept;
    using Write::on_stop_reading;
    using Write::on_stoped_writing;

    ~node_in_x_out_x() noexcept = default;
};

using node_in_1_out_0 = node_in_x_out_x<node_impl_read_1, node_impl_write_0>;
using node_in_0_out_1 = node_in_x_out_x<node_impl_read_0, node_impl_write_1>;
using node_in_1_out_1 = node_in_x_out_x<node_impl_read_1, node_impl_write_1>;
using node_in_0_out_n = node_in_x_out_x<node_impl_read_0, node_impl_write_n>;
using node_in_1_out_n = node_in_x_out_x<node_impl_read_1, node_impl_write_n>;

std::unique_ptr<node_base_t> make_node(std::istream& in, const char* node_id, std::uint16_t host_id) {
    const graph_t graph = load_graph(in);
    const auto this_node_descriptor = get_this_node_descriptor(graph, node_id);

    const auto edges_out = boost::out_edges(
        this_node_descriptor,
        graph
    );

    const auto edges_in = boost::in_edges(
        this_node_descriptor,
        graph
    );

    enum class node_types_t: unsigned {
        IN_0_OUT_0 = 0,
        IN_0_OUT_1 = 1,
        IN_0_OUT_N = 2,
        IN_1_OUT_0 = 1 << 2,
        IN_N_OUT_0 = 2 << 2,
        IN_1_OUT_1 = IN_1_OUT_0 | IN_0_OUT_1,
        IN_1_OUT_N = IN_1_OUT_0 | IN_0_OUT_N,
        IN_N_OUT_N = IN_N_OUT_0 | IN_0_OUT_N,
    };

    const node_types_t type = static_cast<node_types_t>(
        std::min<unsigned>(edges_out.second - edges_out.first, 2)
        | (std::min<unsigned>(edges_in.second - edges_in.first, 2) << 2)
    );

    switch(type) {
    case node_types_t::IN_1_OUT_0: return boost::make_unique<node_in_1_out_0>(graph, node_id, host_id);
    case node_types_t::IN_0_OUT_1: return boost::make_unique<node_in_0_out_1>(graph, node_id, host_id);
    case node_types_t::IN_1_OUT_1: return boost::make_unique<node_in_1_out_1>(graph, node_id, host_id);
    case node_types_t::IN_0_OUT_N: return boost::make_unique<node_in_0_out_n>(graph, node_id, host_id);
    case node_types_t::IN_1_OUT_N: return boost::make_unique<node_in_1_out_n>(graph, node_id, host_id);

    // TODO: for testing only! This is wrong!
    case node_types_t::IN_N_OUT_0: return boost::make_unique<node_in_1_out_0>(graph, node_id, host_id);

    default:
        BOOST_ASSERT_MSG(false, "Error in make_node function - not all node types are handled");
    }

    return {};
}

}
