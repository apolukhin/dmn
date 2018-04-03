#include "node_base.hpp"

#include "load_graph.hpp"
#include <istream>

#include "impl/node_parts/read_0.hpp"
#include "impl/node_parts/read_1.hpp"
#include "impl/node_parts/read_n.hpp"
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

node_base_t::node_base_t(graph_t&& in, const char* node_id, std::uint16_t host_id)
    : config(std::move(in))
    , this_node_descriptor(get_this_node_descriptor(config, node_id))
    , this_node(config[this_node_descriptor])
    , host_id_(host_id)
{}

std::uint16_t node_base_t::edge_id_for_receiver(std::uint16_t out_edge_index) {
    auto edges_out = boost::out_edges(
        this_node_descriptor,
        config
    );

    BOOST_ASSERT_MSG(edges_out.second - edges_out.first >= out_edge_index, "Attempt to get edge_id for receiver failed, because out_edge index is wrong");
    std::advance(edges_out.first, out_edge_index);
    const auto out_vertex_desc = boost::target(*edges_out.first, config);
    const auto edges_in = boost::in_edges(
        out_vertex_desc,
        config
    );

    const auto it = std::find_if(edges_in.first, edges_in.second, [this](graph_t::edge_descriptor edge) {
        const auto desc = boost::source(edge, config);
        return desc == this_node_descriptor;
    });
    BOOST_ASSERT_MSG(it != edges_in.second, "Failed to find edge_id for receiver");
    return static_cast<std::uint16_t>(it - edges_in.first);
}

std::uint16_t node_base_t::count_in_edges() const noexcept {
    const auto edges_in = boost::in_edges(
        this_node_descriptor,
        config
    );
    const std::size_t edges_in_count = edges_in.second - edges_in.first;
    BOOST_ASSERT_MSG(edges_in_count > 1, "Incorrect node class used for dealing with muliple out edges. Error in make_node() function");

    return static_cast<std::uint16_t>(edges_in_count);
}

std::uint16_t node_base_t::count_in_edges_for_receiver(std::uint16_t out_edge_index) const noexcept {
    auto edges_out = boost::out_edges(
        this_node_descriptor,
        config
    );

    BOOST_ASSERT_MSG(edges_out.second - edges_out.first >= out_edge_index, "Attempt to get edge_id for receiver failed, because out_edge index is wrong");
    std::advance(edges_out.first, out_edge_index);
    const auto out_vertex_desc = boost::target(*edges_out.first, config);
    const auto edges_in = boost::in_edges(
        out_vertex_desc,
        config
    );

    return static_cast<std::uint16_t>(edges_in.second - edges_in.first);
}

std::uint16_t node_base_t::count_out_edges() const noexcept {
    const auto edges_out = boost::out_edges(
        this_node_descriptor,
        config
    );
    const std::size_t edges_out_count = edges_out.second - edges_out.first;
    BOOST_ASSERT_MSG(edges_out_count > 1, "Incorrect node class used for dealing with muliple out edges. Error in make_node() function");

    return static_cast<std::uint16_t>(edges_out_count);
}

packet_t node_base_t::call_callback(packet_t packet) noexcept {
    stream_t s{*this, std::move(packet)};

    node_base_t::callback_(s);
    return s.move_out_data();
}

node_base_t::~node_base_t() noexcept = default;





template <class Read, class Write>
struct node_in_x_out_x final: Read, Write {
    node_in_x_out_x(graph_t&& in, const char* node_id, std::uint16_t host_id)
        : node_base_t(std::move(in), node_id, host_id)
        , Read()
        , Write()
    {}

    using Read::on_stop_reading;
    using Write::on_packet_accept;
    using Write::on_stop_reading;
    using Write::on_stoped_writing;

    ~node_in_x_out_x() noexcept = default;
};

using node_in_0_out_1 = node_in_x_out_x<node_impl_read_0, node_impl_write_1>;
using node_in_0_out_n = node_in_x_out_x<node_impl_read_0, node_impl_write_n>;

using node_in_1_out_0 = node_in_x_out_x<node_impl_read_1, node_impl_write_0>;
using node_in_1_out_1 = node_in_x_out_x<node_impl_read_1, node_impl_write_1>;
using node_in_1_out_n = node_in_x_out_x<node_impl_read_1, node_impl_write_n>;

using node_in_n_out_0 = node_in_x_out_x<node_impl_read_n, node_impl_write_0>;
using node_in_n_out_1 = node_in_x_out_x<node_impl_read_n, node_impl_write_1>;
using node_in_n_out_n = node_in_x_out_x<node_impl_read_n, node_impl_write_n>;


std::unique_ptr<node_base_t> make_node(const std::string& in, const char* node_id, std::uint16_t host_id) {
    graph_t graph = load_graph(in);
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
        IN_N_OUT_1 = IN_N_OUT_0 | IN_0_OUT_1,
        IN_N_OUT_N = IN_N_OUT_0 | IN_0_OUT_N,
    };

    const node_types_t type = static_cast<node_types_t>(
        std::min<unsigned>(edges_out.second - edges_out.first, 2)
        | (std::min<unsigned>(edges_in.second - edges_in.first, 2) << 2)
    );

    switch(type) {
    case node_types_t::IN_1_OUT_0: return boost::make_unique<node_in_1_out_0>(std::move(graph), node_id, host_id);
    case node_types_t::IN_0_OUT_1: return boost::make_unique<node_in_0_out_1>(std::move(graph), node_id, host_id);
    case node_types_t::IN_1_OUT_1: return boost::make_unique<node_in_1_out_1>(std::move(graph), node_id, host_id);
    case node_types_t::IN_0_OUT_N: return boost::make_unique<node_in_0_out_n>(std::move(graph), node_id, host_id);
    case node_types_t::IN_1_OUT_N: return boost::make_unique<node_in_1_out_n>(std::move(graph), node_id, host_id);

    // TODO: This is currently incorrectly covered in tests! Tests must be fixed!!!
    case node_types_t::IN_N_OUT_0: return boost::make_unique<node_in_n_out_0>(std::move(graph), node_id, host_id);
    case node_types_t::IN_N_OUT_1: return boost::make_unique<node_in_n_out_1>(std::move(graph), node_id, host_id);
    case node_types_t::IN_N_OUT_N: return boost::make_unique<node_in_n_out_n>(std::move(graph), node_id, host_id);

    default:
        BOOST_ASSERT_MSG(false, "Error in make_node function - not all node types are handled");
    }

    return {};
}

}
