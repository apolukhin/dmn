#pragma once

#include <boost/graph/adjacency_list.hpp>

namespace dmn {

struct hosts_strong_t: private std::vector<std::pair<std::string, unsigned short>> {
    using base_t = std::vector<std::pair<std::string, unsigned short>>;

    using base_t::value_type;
    using base_t::iterator;
    using base_t::const_iterator;

    using base_t::base_t;
    using base_t::begin;
    using base_t::end;
    using base_t::operator[];
    using base_t::front;
    using base_t::push_back;
    using base_t::empty;
    using base_t::swap;
    using base_t::size;

    base_t& base() noexcept { return *this; }
    const base_t& base() const noexcept { return *this; }
};

struct vertex_t {
    std::string node_id;
    hosts_strong_t hosts;
};

using graph_t = boost::adjacency_list<
    boost::vecS
    , boost::vecS
    , boost::bidirectionalS
    , vertex_t
>;

graph_t load_graph(const std::string& in);

constexpr std::size_t max_in_or_out_edges_per_node = 11000;

}
