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

namespace {

std::atomic<unsigned> sequence_counter{0};
constexpr unsigned max_seq = 129;
const std::map<unsigned, unsigned> seq_ethalon = []() {
    std::map<unsigned, unsigned> res;
    for (unsigned i = 0; i <= max_seq; ++i) {
        res[i] = 1;
    }
    return res;
}();

std::mutex  seq_mutex;
std::map<unsigned, unsigned> sequences;

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
    ++sequences[seq];
}

void resend_sequence(dmn::stream_t& s) {
    const auto data = s.get_data("seq");
    s.add(data.first, data.second, "seq");
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(make_nodes_end_to_end) {
    sequence_counter = 0;
    sequences.clear();
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            a -> b;
        }
    )"};

    auto node_a = dmn::make_node(g, "a", 0);
    BOOST_TEST(!!node_a);
    node_a->callback_ = generate_sequence;

    auto node_b = dmn::make_node(g, "b", 0);
    BOOST_TEST(!!node_b);
    node_b->callback_ = remember_sequence;


    tests::shutdown_nodes(node_a, node_b);

    //std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<unsigned>(std::cerr, " "));
    BOOST_CHECK(sequences == seq_ethalon);
}


BOOST_AUTO_TEST_CASE(make_nodes_chain_end_to_end) {
    sequence_counter = 0;
    sequences.clear();
    const std::string g{R"(
        digraph test
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
        auto node = dmn::make_node(g, name, 0);
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


// TODO: this test is wrong! It is temporary, just to test write_n node
BOOST_AUTO_TEST_CASE(make_nodes_broadcast_2_end_to_end) {
    sequence_counter = 0;
    sequences.clear();
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44002"];
            c [hosts = "127.0.0.1:44003"];
            d [hosts = "127.0.0.1:44004"];
            a -> b -> d;
            a -> c -> d;
        }
    )"};

    auto make_node = [g](const char* name, auto callback) {
        auto node = dmn::make_node(g, name, 0);
        BOOST_TEST(!!node);
        node->callback_ = callback;
        return node;
    };

    tests::shutdown_nodes(
        make_node("a", generate_sequence),
        make_node("b", resend_sequence),
        make_node("c", resend_sequence),
        make_node("d", remember_sequence)
    );

    auto ethalon = seq_ethalon;
    for (auto& v : ethalon) {
        ++v.second;
    }
    //std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<unsigned>(std::cerr, " "));
    BOOST_CHECK(sequences == ethalon);
}


// TODO: this test is wrong! It is temporary, just to test write_n node
BOOST_AUTO_TEST_CASE(make_nodes_broadcast_10_end_to_end) {
    sequence_counter = 0;
    sequences.clear();
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b0 [hosts = "127.0.0.1:44002"];
            b1 [hosts = "127.0.0.1:44003"];
            b2 [hosts = "127.0.0.1:44004"];
            b3 [hosts = "127.0.0.1:44005"];
            b4 [hosts = "127.0.0.1:44006"];
            b5 [hosts = "127.0.0.1:44007"];
            b6 [hosts = "127.0.0.1:44008"];
            b7 [hosts = "127.0.0.1:44009"];
            b8 [hosts = "127.0.0.1:44010"];
            b9 [hosts = "127.0.0.1:44011"];
            c [hosts = "127.0.0.1:44012"];
            a -> b0 -> c;
            a -> b1 -> c;
            a -> b2 -> c;
            a -> b3 -> c;
            a -> b4 -> c;
            a -> b5 -> c;
            a -> b6 -> c;
            a -> b7 -> c;
            a -> b8 -> c;
            a -> b9 -> c;
        }
    )"};

    auto make_node = [g](const char* name, auto callback) {
        auto node = dmn::make_node(g, name, 0);
        BOOST_TEST(!!node);
        node->callback_ = callback;
        return node;
    };

    tests::shutdown_nodes(
        make_node("a", generate_sequence),
        make_node("b0", resend_sequence),
        make_node("b1", resend_sequence),
        make_node("b2", resend_sequence),
        make_node("b3", resend_sequence),
        make_node("b4", resend_sequence),
        make_node("b5", resend_sequence),
        make_node("b6", resend_sequence),
        make_node("b7", resend_sequence),
        make_node("b8", resend_sequence),
        make_node("b9", resend_sequence),
        make_node("c", remember_sequence)
    );

    auto ethalon = seq_ethalon;
    for (auto& v : ethalon) {
        v.second += 9;
    }
    //std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<unsigned>(std::cerr, " "));
    BOOST_CHECK(sequences == ethalon);
}
