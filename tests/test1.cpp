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
    BOOST_TEST(boost::num_vertices(dmn::load_graph(ss)) == 3);


    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> c;
           c -> b;
        }
    )");
    BOOST_CHECK_THROW(dmn::load_graph(ss), std::runtime_error);


    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> a;
           b -> c;
           a -> c;
        }
    )");
    BOOST_CHECK_THROW(dmn::load_graph(ss), std::runtime_error);



    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> c;
           c -> b;
           c -> d;
        }
    )");
    BOOST_CHECK_THROW(dmn::load_graph(ss), std::runtime_error);
}
