
#include "load_graph.hpp"

#define BOOST_GRAPH_USE_SPIRIT_PARSER
#include <boost/graph/graphviz.hpp>
#include <boost/graph/hawick_circuits.hpp>
#include <boost/optional.hpp>
#include <iostream>

namespace dmn {

namespace {

struct on_circut {
    template <class T, class Graph>
    void cycle(const T&, const Graph& g) const {
        throw std::runtime_error("Graph has a circut");
    }
};

void validate_flow_network(const graph_t& graph) {
    using vertex_descriptor_t = boost::graph_traits<graph_t>::vertex_descriptor;
    boost::optional<vertex_descriptor_t> source, sink;
    const auto all_vertices = vertices(graph);
    for (auto vp = all_vertices; vp.first != vp.second; ++vp.first) {
        const vertex_descriptor_t v = *vp.first;
        const auto edges_out = boost::out_edges(v, graph);
        const bool has_no_out_edges = (edges_out.first == edges_out.second);

        const auto edges_in = boost::in_edges(v, graph);
        const bool has_no_in_edges = (edges_in.first == edges_in.second);

        if (has_no_in_edges && has_no_out_edges) {
            throw std::runtime_error(
                "Graph has a dangling vertex '" + graph[v].node_id + "'"
            );
        }

        if (has_no_in_edges) {
            if (source) {
                throw std::runtime_error(
                    "Graph has multiple source vertexes: '" + graph[v].node_id + "' and '" + graph[*source].node_id + "'"
                );
            }
            source = v;
        }

        if (has_no_out_edges) {
            if (sink) {
                throw std::runtime_error(
                    "Graph has multiple source vertexes: '" + graph[v].node_id + "' and '" + graph[*sink].node_id + "'"
                );
            }
            sink = v;
        }
    }

    if (!sink) {
        throw std::runtime_error("Graph has no sink");
    }

    if (!source) {
        throw std::runtime_error("Graph has no source");
    }

    hawick_circuits(graph, on_circut{});
}

}

graph_t load_graph(std::istream& in) {
    graph_t graph;

    boost::dynamic_properties dp;
    dp.property("node_id", boost::get(&vertex_t::node_id, graph));
    dp.property("hosts", boost::get(&vertex_t::hosts, graph));

    boost::read_graphviz(in, graph, dp);
    validate_flow_network(graph);

    return graph;
}


}
