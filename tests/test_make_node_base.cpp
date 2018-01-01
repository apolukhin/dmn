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

void noop(dmn::stream_t& e){
    e.stop();
}

BOOST_AUTO_TEST_CASE(make_nodes_base) {
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            a -> b;
        }
    )"};

    auto node_a = dmn::make_node(g + " ", "a", 0);
    BOOST_TEST(!!node_a);
    node_a->callback_ = &noop;

    auto node_b = dmn::make_node(" " + g, "b", 0);
    BOOST_TEST(!!node_b);
    node_b->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    tests::shutdown_nodes(node_a, node_b);
}

BOOST_AUTO_TEST_CASE(make_nodes_base_multiple_hosts_1) {
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001;127.0.0.1:44002"];
            b [hosts = "127.0.0.1:44005"];
            a -> b;
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

    tests::shutdown_nodes(node_a, node_b);
}

BOOST_AUTO_TEST_CASE(make_nodes_base_multiple_hosts_2) {
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002; 127.0.0.1:44005"];
            a -> b;
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

    tests::shutdown_nodes(node_a, node_b);
}

BOOST_AUTO_TEST_CASE(make_nodes_chain_base) {
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            c [hosts = "127.0.0.1:44003"];
            a -> b;
            b -> c;
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

    tests::shutdown_nodes(node_a, node_b, node_c);
}

BOOST_AUTO_TEST_CASE(make_nodes_long_chain_base) {
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            c [hosts = "127.0.0.1:44003"];
            d [hosts = "127.0.0.1:44004"];
            a -> b;
            b -> c;
            c -> d;
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
