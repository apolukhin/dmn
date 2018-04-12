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
namespace {

std::atomic<unsigned> sequence_counter{0};
constexpr unsigned max_seq = 257;
std::map<unsigned, unsigned> seq_ethalon(unsigned max_value = max_seq) {
    std::map<unsigned, unsigned> res;
    for (unsigned i = 0; i <= max_value; ++i) {
        res[i] = 1;
    }
    return res;
}

std::mutex  seq_mutex;
std::map<unsigned, unsigned> sequences;

void generate_sequence(dmn::stream_t& s) {
    const unsigned seq = sequence_counter.fetch_add(1);

    if (seq <= max_seq) {
        auto data = boost::lexical_cast<std::string>(seq);
        s.add(data.data(), data.size(), "seq");
    }

    if (seq >= max_seq) {
        s.stop();
    }
}

void remember_sequence(dmn::stream_t& s) {
    for (const char* data_type: {"seq", "seq0", "seq1", "seq2", "seq3", "seq4", "seq5", "seq6", "seq7", "seq8", "seq9", "seq10"}) {
        const auto data = s.get_data(data_type);
        if (data.second == 0) {
            continue;
        }
        unsigned seq = 0;
        const bool res = boost::conversion::try_lexical_convert<unsigned>(static_cast<const unsigned char*>(data.first), data.second, seq);
        BOOST_CHECK(res);
        std::lock_guard<std::mutex> l(seq_mutex);
        ++sequences[seq];
    }
}

void resend_sequence(dmn::stream_t& s) {
    const auto data = s.get_data("seq");
    s.add(data.first, data.second, "seq");
}

template <unsigned Shift>
void resend_shifted_sequence(dmn::stream_t& s) {
    const auto data = s.get_data("seq");
    if (data.second == 0) {
        return;
    }

    unsigned seq = 0;
    const bool res = boost::conversion::try_lexical_convert<unsigned>(static_cast<const unsigned char*>(data.first), data.second, seq);
    if (!res) {
        BOOST_CHECK(res);
    }

    seq += (Shift ? Shift * max_seq + 1 : 0);
    std::string value = boost::lexical_cast<std::string>(seq);
    s.add(value.c_str(), value.size(), ("seq" + boost::lexical_cast<std::string>(Shift)).c_str());
}

} // anonymous namespace







BOOST_AUTO_TEST_CASE(make_nodes_end_to_end_5) {
    sequence_counter = 0;
    sequences.clear();
    const std::string g{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44006;127.0.0.1:44007;127.0.0.1:44008;127.0.0.1:44009;127.0.0.1:44010;"];
            b [hosts = "127.0.0.1:44001; 127.0.0.1:44002; 127.0.0.1:44003;127.0.0.1:44004;127.0.0.1:44005"];
            a -> b;
        }
    )"};

    std::unique_ptr<dmn::node_base_t> node_a_5[5] = {
        dmn::make_node(g, "a", 0),
        dmn::make_node(g, "a", 1),
        dmn::make_node(g, "a", 2),
        dmn::make_node(g, "a", 3),
        dmn::make_node(g, "a", 4)
    };
    for (auto& v: node_a_5) {
        BOOST_TEST(!!v);
        v->callback_ = generate_sequence;
    }

    std::unique_ptr<dmn::node_base_t> node_b_5[5] = {
        dmn::make_node(g, "b", 0),
        dmn::make_node(g, "b", 1),
        dmn::make_node(g, "b", 2),
        dmn::make_node(g, "b", 3),
        dmn::make_node(g, "b", 4)
    };
    for (auto& v: node_b_5) {
        BOOST_TEST(!!v);
        v->callback_ = remember_sequence;
    }

    tests::shutdown_nodes(
            node_a_5[0], node_a_5[1], node_a_5[2], node_a_5[3], node_a_5[4],
            node_b_5[0], node_b_5[1], node_b_5[2], node_b_5[3], node_b_5[4]
    );

    //std::copy(sequences.begin(), sequences.end(), std::ostream_iterator<unsigned>(std::cerr, " "));
    BOOST_CHECK(sequences == seq_ethalon());
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
    BOOST_CHECK(sequences == seq_ethalon());
}


BOOST_AUTO_TEST_CASE(make_nodes_duplicate_2_end_to_end) {
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


    //std::for_each(sequences.begin(), sequences.end(), [](auto val){ std::cerr << '{' << val.first << ',' << val.second << '}'; });
    BOOST_CHECK(sequences == seq_ethalon());
}

BOOST_AUTO_TEST_CASE(make_nodes_duplicate_10_end_to_end) {
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

    //std::for_each(sequences.begin(), sequences.end(), [](auto val){ std::cerr << '{' << val.first << ',' << val.second << '}'; });
    BOOST_CHECK(sequences == seq_ethalon());
}

BOOST_AUTO_TEST_CASE(make_nodes_combine_2_end_to_end) {
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
        make_node("b", resend_shifted_sequence<0>),
        make_node("c", resend_shifted_sequence<1>),
        make_node("d", remember_sequence)
    );


    //std::for_each(sequences.begin(), sequences.end(), [](auto val){ std::cerr << '{' << val.first << ',' << val.second << '}'; });
    BOOST_CHECK(sequences == seq_ethalon(max_seq * 2 + 1));
}*/
