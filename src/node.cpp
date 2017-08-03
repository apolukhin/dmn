#include "node.hpp"
#include "load_graph.hpp"

#include <istream>
#include <boost/asio/io_service.hpp>

namespace dmn {

struct node_t::impl_t {
    const graph_t config;
    const boost::graph_traits<graph_t>::vertex_descriptor this_node_descriptor;
    const vertex_t* this_node;

    boost::asio::io_service ios;
};

node_t::node_t(std::istream& in, boost::string_view node_id) {
    const auto g = load_graph(in);

    const auto vds = vertices(g);
    const auto it_descriptor = std::find_if(vds.first, vds.second, [node_id, &g](auto v_descriptor){
        return g[v_descriptor].node_id == node_id;
    });
    if (it_descriptor == vds.second) {
        throw std::runtime_error("Node with id '" + std::string(node_id) + "' does not exist in graph");
    }

    new (&data) impl_t {
        g,  // intentionally copying to shrink_to_fit() all internals
        *it_descriptor,
    };

    impl().this_node = std::addressof(
        impl().config[impl().this_node_descriptor]
    );
}

node_t::~node_t() noexcept {
    impl().~impl_t();
}


boost::asio::io_service& node_t::ios() noexcept {
    return impl().ios;
}

}
