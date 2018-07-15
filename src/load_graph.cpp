#include "load_graph.hpp"

#include <boost/graph/graphviz.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/visitors.hpp>

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>

#include <istream>
#include <ostream>

namespace dmn {

namespace {

template <typename VertexDesc>
struct check_for_dag_visitor : public boost::dfs_visitor<> {
    explicit check_for_dag_visitor(std::vector<VertexDesc>& paths)
        : paths_(paths)
    {}

    template <typename Edge, typename Graph>
    void back_edge(const Edge& e, Graph& g) {
        auto v_target = boost::target(e, g);
        auto v_source = boost::source(e, g);

        std::string msg = "Graph has a circut: ";
        auto it = std::find(paths_.cbegin(), paths_.cend(), v_target);
        BOOST_ASSERT_MSG(it != paths_.cend(), "Boost.Graph has not put a discovered vertex into the path");

        for (;*it != v_source; ++it) {
            msg += g[boost::vertex(*it, g)].node_id;
            msg += " -> ";
        }
        msg += g[boost::vertex(v_source, g)].node_id;
        msg += " -> ";
        msg += g[boost::vertex(v_target, g)].node_id;

        throw std::runtime_error(std::move(msg));
    }

    template <typename Vertex, typename Graph>
    void discover_vertex(const Vertex& u, Graph&) {
        paths_.push_back(u);
    }

    template <typename Vertex, typename Graph>
    void finish_vertex(const Vertex& u, Graph&) {
        BOOST_ASSERT_MSG(paths_.back() == u, "Boost.Graph called finish_vertex on a vertex that was not put last via discover_vertex");
        paths_.pop_back();
        (void)u;
    }

    std::vector<VertexDesc>& paths_;
};


template <typename VertexListGraph>
inline void check_for_dag(VertexListGraph& g) {
    using vd_t = typename boost::graph_traits<VertexListGraph>::vertex_descriptor;
    std::vector<vd_t> paths;
    paths.reserve(boost::num_vertices(g));
    const boost::bgl_named_params<int, boost::buffer_param_t> params{};
    boost::depth_first_search(
        g,
        params.visitor(check_for_dag_visitor<vd_t>{paths})
    );
}


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

    check_for_dag(graph);
}

void validate_vertexes(const graph_t& graph) {
    const auto all_vertices = boost::vertices(graph);
    for (auto vp = all_vertices; vp.first != vp.second; ++vp.first) {
        const vertex_t& v = graph[*vp.first];

        const auto edges_in = boost::in_edges(*vp.first, graph);
        if (edges_in.second - edges_in.first > max_in_or_out_edges_per_node) {
            throw std::runtime_error(
                "Each vertex must have at most "
                + std::to_string(max_in_or_out_edges_per_node)
                + " incomming edges. Vertex '" + v.node_id + "' has "
                + std::to_string(edges_in.second - edges_in.first)
                + " incomming edges."
            );
        }

        const auto edges_out = boost::out_edges(*vp.first, graph);
        if (edges_out.second - edges_out.first > max_in_or_out_edges_per_node) {
            throw std::runtime_error(
                "Each vertex must have at most "
                + std::to_string(max_in_or_out_edges_per_node)
                + " outgoing edges. Vertex '" + v.node_id + "' has "
                + std::to_string(edges_out.second - edges_out.first)
                + " outgoing edges."
            );
        }
    }
}

void validate_hosts(const graph_t& graph) {
    const auto all_vertices = vertices(graph);
    std::unordered_map<std::pair<std::string, unsigned short>, std::size_t, boost::hash<std::pair<std::string, unsigned short>> > hosts_vertex;

    for (auto vp = all_vertices; vp.first != vp.second; ++vp.first) {
        const vertex_t& v = graph[*vp.first];

        if (v.hosts.empty()) {
            throw std::runtime_error(
                "Vertex '" + v.node_id + "' and all other vertexes must have non empty 'hosts' property. Example:\n"
                "(digraph example {\n"
                "    a [hosts = \"127.0.0.1:44001\"];\n"
                "    b [hosts = \"127.0.0.1:44003\"];\n"
                "    a -> b;\n"
                "}"
            );
        }

        for (const auto& h: v.hosts.base()) {
            if (hosts_vertex.count(h)) {
                throw std::runtime_error(
                    "Same host:port for vertexes '" + graph[hosts_vertex[h]].node_id + "' and '" + v.node_id + "'"
                );
            }

            hosts_vertex[h] = *vp.first;
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
            boost::trim_left_copy(v.substr(0, v.find(':'))),
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

graph_t load_graph(const std::string& in) {
    graph_t graph;
    {
        boost::dynamic_properties dp;
        dp.property("node_id", boost::get(&vertex_t::node_id, graph));
        dp.property("hosts", boost::get(&vertex_t::hosts, graph));
        boost::read_graphviz(in.begin(), in.end(), graph, dp);
    }
    validate_flow_network(graph);
    validate_vertexes(graph);
    validate_hosts(graph);

    return graph;
}


}
