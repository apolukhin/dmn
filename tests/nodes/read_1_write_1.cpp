#include "node_base.hpp"
#include "stream.hpp"

#include <mutex>
#include <vector>
#include <set>
#include <numeric>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "nodes_tester.hpp"
#include <boost/test/data/test_case.hpp>

namespace {

std::atomic<unsigned> sequence_counter{0};
constexpr unsigned max_seq = 10;
std::map<unsigned, unsigned> seq_ethalon(unsigned max_value = max_seq, unsigned repetitions = 1) {
    std::map<unsigned, unsigned> res;
    for (unsigned i = 0; i <= max_value; ++i) {
        res[i] = repetitions;
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

} // anonymous namespace

BOOST_DATA_TEST_CASE(read_1_write_1_simple_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    sequence_counter = 0;
    sequences.clear();

    tests::nodes_tester_t{
        "a -> b -> c",
        {
            {"a", &generate_sequence, hosts_count},
            {"b", &resend_sequence, hosts_count},
            {"c", &remember_sequence, hosts_count},
        }
    }
    .threads(threads_count)
    .poll();

    BOOST_TEST(sequences == seq_ethalon());
}
