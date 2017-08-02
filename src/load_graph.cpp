
#include "load_graph.hpp"

#define BOOST_GRAPH_USE_SPIRIT_PARSER
#include <boost/graph/graphviz.hpp>
#include <iostream>

namespace dmn {

graph_t load_graph(std::istream& in) {
    graph_t res;

    boost::dynamic_properties dp;
    dp.property("node_id", boost::get(&vertex_t::node_id, res));
    dp.property("hosts", boost::get(&vertex_t::hosts, res));

    boost::read_graphviz(in, res, dp);

    return res;
}


}
