#include "nodes_tester.hpp"

BOOST_AUTO_TEST_SUITE(read_1_write_1)

BOOST_DATA_TEST_CASE(simple_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    nodes_tester_t{
        "a -> b -> c",
        {
            {"a", actions::generate, hosts_count},
            {"b", actions::resend, hosts_count},
            {"c", actions::remember, hosts_count},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

// TODO: permutations for nodes start
BOOST_DATA_TEST_CASE(chain_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    nodes_tester_t{
        "a -> b0 -> b1 -> b2 -> b3 -> b4 -> c",
        {
            {"a", actions::generate, hosts_count},
            {"b0", actions::resend, hosts_count},
            {"b1", actions::resend, hosts_count},
            {"b2", actions::resend, hosts_count},
            {"b3", actions::resend, hosts_count},
            {"b4", actions::resend, hosts_count},
            {"c", actions::remember, hosts_count},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}


BOOST_AUTO_TEST_SUITE_END()
