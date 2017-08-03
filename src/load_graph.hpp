#pragma once

#include <boost/graph/adjacency_list.hpp>

namespace dmn {

struct vertex_t {
    std::string node_id;
    std::string hosts;
};

using graph_t = boost::adjacency_list<
    boost::vecS
    , boost::vecS
    , boost::bidirectionalS
    , vertex_t
>;

graph_t load_graph(std::istream& in);

}
