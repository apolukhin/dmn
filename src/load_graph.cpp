
#include "load_graph.hpp"

#define BOOST_GRAPH_USE_SPIRIT_PARSER
#include <boost/graph/graphviz.hpp>
#include <boost/graph/hawick_circuits.hpp>
#include <boost/optional.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>

#include <istream>
#include <ostream>

namespace dmn {

namespace {

struct on_circut {
    template <class T, class Graph>
    void cycle(const T& container, const Graph& g) const {
        std::string msg = "Graph has a circut: ";
        for (const auto& v: container) {
            msg += g[v].node_id;
            msg += " -> ";
        }
        msg += g[container.front()].node_id;
        throw std::runtime_error(std::move(msg));
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
        throw std::runtime_error("Graph has no sink! There must be one vertex with incomming edges and no outgoing edges.");
    }

    if (!source) {
        throw std::runtime_error("Graph has no source! There must be one vertex with outgoing edges and no incomming edges.");
    }

    hawick_circuits(graph, on_circut{});
}

void validate_vertexes(const graph_t& graph) {
    const auto all_vertices = vertices(graph);
    for (auto vp = all_vertices; vp.first != vp.second; ++vp.first) {
        const vertex_t& v = graph[*vp.first];

        if (v.hosts.empty()) {
            throw std::runtime_error(
                "Each vertex must have a non empty hosts property. Example:\n"
                "(digraph graph {\n"
                "    a [hosts = '127.0.0.1:44001'];\n"
                "    b [hosts = '127.0.0.1:44003'];\n"
                "    a -> b;\n"
                "}"
            );
        }
    }

}

} // anonymous namespace

static std::istream& operator>>(std::istream& in, hosts_strong_t& hosts) {
    std::string hosts_raw(std::istreambuf_iterator<char>(in), {});

    std::vector<std::string> hosts_with_port;
    boost::split(hosts_with_port, hosts_raw, boost::is_any_of(";"), boost::token_compress_on);
    for (auto& v: hosts_with_port) {
        const auto delim = v.find(':');
        hosts.base().emplace_back(
            v.substr(0, v.find(':')),
            (delim == std::string::npos ? 63101 : boost::lexical_cast<unsigned short>(v.substr(delim + 1)))
        );
    }
    return in;
}

static std::ostream& operator<<(std::ostream& os, const hosts_strong_t& hosts) {
    for (const auto& h : hosts.base()) {
        os << h.first << ':' << h.second << ';';
    }
    return os;
}

graph_t load_graph(std::istream& in) {
    graph_t graph;

    boost::dynamic_properties dp;
    dp.property("node_id", boost::get(&vertex_t::node_id, graph));
    dp.property("hosts", boost::get(&vertex_t::hosts, graph));

    boost::read_graphviz(in, graph, dp);
    validate_flow_network(graph);
    validate_vertexes(graph);

    return graph;
}


}
