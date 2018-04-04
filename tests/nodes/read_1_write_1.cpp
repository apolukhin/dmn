#include "nodes_tester.hpp"

BOOST_DATA_TEST_CASE(read_1_write_1_simple_hosts_x_threads,
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
    .poll();
}
