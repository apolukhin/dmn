#include "load_graph.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <sstream>


BOOST_AUTO_TEST_CASE(test_no_1) {
    std::cout << "Hello World!" << std::endl;
}

BOOST_AUTO_TEST_CASE(test_graph_loading) {
    std::stringstream ss{R"(
        digraph graph
        {
            a [hosts = "127.0.0.1:44001"];
            c [hosts = "127.0.0.1:44002"];
            b [hosts = "127.0.0.1:44003"];
            a -> b;
            b -> c;
        }
    )"};
    const auto g = dmn::load_graph(ss);
    BOOST_TEST(boost::num_vertices(g) == 3);
}
