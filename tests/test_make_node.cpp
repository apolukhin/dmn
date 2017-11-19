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
    std::string g{R"(
        digraph graph
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            a -> b;
        }
    )"};

    std::stringstream ss{g};
    auto node_a = dmn::make_node(ss, "a");
    BOOST_TEST(!!node_a);
    node_a->callback_ = &noop;

    ss.str(g);
    auto node_b = dmn::make_node(ss, "b");
    BOOST_TEST(!!node_b);
    node_b->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    tests::shutdown_nodes(node_a, node_b);
}

BOOST_AUTO_TEST_CASE(make_nodes_chain_base) {
    std::string g{R"(
        digraph graph
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            c [hosts = "127.0.0.1:44003"];
            a -> b;
            b -> c;
        }
    )"};

    std::stringstream ss{g};
    auto node_a = dmn::make_node(ss, "a");
    BOOST_TEST(!!node_a);
    node_a->callback_ = &noop;

    ss.str(g);
    auto node_b = dmn::make_node(ss, "b");
    BOOST_TEST(!!node_b);
    node_b->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    ss.str(g);
    auto node_c = dmn::make_node(ss, "c");
    BOOST_TEST(!!node_c);
    node_c->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    tests::shutdown_nodes(node_a, node_b, node_c);
}

BOOST_AUTO_TEST_CASE(make_nodes_long_chain_base) {
    std::string g{R"(
        digraph graph
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

    std::stringstream ss{g};
    auto node_a = dmn::make_node(ss, "a");
    BOOST_TEST(!!node_a);
    node_a->callback_ = &noop;

    ss.str(g);
    auto node_b = dmn::make_node(ss, "b");
    BOOST_TEST(!!node_b);
    node_b->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    ss.str(g);
    auto node_c = dmn::make_node(ss, "c");
    BOOST_TEST(!!node_c);
    node_c->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    ss.str(g);
    auto node_d = dmn::make_node(ss, "d");
    BOOST_TEST(!!node_d);
    node_d->callback_ = [](dmn::stream_t& e){
        e.stop();
    };

    tests::shutdown_nodes(node_a, node_b, node_c, node_d);
}

namespace {

std::atomic<unsigned> sequence_counter{0};
constexpr unsigned max_seq = 129;
const std::set<unsigned> seq_ethalon = []() {
    std::vector<unsigned> v;
    v.resize(max_seq + 1);
    std::iota(v.begin(), v.end(), 0);
    return std::set<unsigned>{v.begin(), v.end()};
}();

std::mutex  seq_mutex;
std::set<unsigned> sequences;

void generate_sequence(dmn::stream_t& s) {
    const unsigned seq = sequence_counter.fetch_add(1);
    auto data = boost::lexical_cast<std::string>(seq);
    s.add(data.data(), data.size(), "seq");

    if (seq == max_seq) {
        //dmn::node_base_t::ios().stop();
        //dmn::node_base_t::ios().reset()
        s.stop();
    }
}

void remember_sequence(dmn::stream_t& s) {
    const auto data = s.get_data("seq");
    unsigned seq = 0;
    const bool res = boost::conversion::try_lexical_convert<unsigned>(static_cast<const unsigned char*>(data.first), data.second, seq);
    BOOST_CHECK(res);
    std::lock_guard<std::mutex> l(seq_mutex);
    sequences.insert(seq);
}

void resend_sequence(dmn::stream_t& s) {
    const auto data = s.get_data("seq");
    s.add(data.first, data.second, "seq");
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(make_nodes_end_to_end) {
    sequence_counter = 0;
    sequences.clear();
    std::string g{R"(
        digraph graph
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            a -> b;
        }
    )"};

    std::stringstream ss{g};
    auto node_a = dmn::make_node(ss, "a");
    BOOST_TEST(!!node_a);
    node_a->callback_ = generate_sequence;

    ss.str(g);
    auto node_b = dmn::make_node(ss, "b");
    BOOST_TEST(!!node_b);
    node_b->callback_ = remember_sequence;


    tests::shutdown_nodes(node_a, node_b);

    //std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<unsigned>(std::cerr, " "));
    BOOST_CHECK(sequences == seq_ethalon);
}


BOOST_AUTO_TEST_CASE(make_nodes_chain_end_to_end) {
    sequence_counter = 0;
    sequences.clear();
    std::string g{R"(
        digraph graph
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            c [hosts = "127.0.0.1:44003"];
            d [hosts = "127.0.0.1:44004"];
            e [hosts = "127.0.0.1:44005"];
            f [hosts = "127.0.0.1:44006"];
            a -> b -> c -> d -> e -> f;
        }
    )"};

    auto make_node = [g](const char* name, auto callback) {
        std::stringstream ss{g};
        auto node = dmn::make_node(ss, name);
        BOOST_TEST(!!node);
        node->callback_ = callback;
        return node;
    };

    tests::shutdown_nodes(
        make_node("a", generate_sequence),
        make_node("b", resend_sequence),
        make_node("c", resend_sequence),
        make_node("d", resend_sequence),
        make_node("e", resend_sequence),
        make_node("f", remember_sequence)
    );

    //std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<unsigned>(std::cerr, " "));
    BOOST_CHECK(sequences == seq_ethalon);
}
