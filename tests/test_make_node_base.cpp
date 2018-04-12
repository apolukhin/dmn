#include "node_base.hpp"

#include "stream.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/io_service.hpp>

#include <mutex>
#include <vector>
#include <set>
#include <numeric>
#include <iostream>

#include "tests_common.hpp"
/*
void noop(dmn::stream_t& e){
    e.stop();
}


BOOST_AUTO_TEST_CASE(make_nodes_bradcast_base) {
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            c [hosts = "127.0.0.1:44003"];
            d [hosts = "127.0.0.1:44004"];
            a -> b -> c;
            a -> d -> c;
        }
    )"};

    auto node_a = dmn::make_node(g, "a", 0);
    BOOST_TEST(!!node_a);
    node_a->callback_ = &noop;

    auto node_b = dmn::make_node(g, "b", 0);
    BOOST_TEST(!!node_b);
    node_b->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    auto node_c = dmn::make_node(g, "c", 0);
    BOOST_TEST(!!node_c);
    node_c->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    auto node_d = dmn::make_node(g, "d", 0);
    BOOST_TEST(!!node_d);
    node_d->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    tests::shutdown_nodes(node_a, node_b, node_c, node_d);
}
*/
