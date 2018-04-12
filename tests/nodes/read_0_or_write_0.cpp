#include "nodes_tester.hpp"

BOOST_AUTO_TEST_SUITE(read_0_or_write_0)

BOOST_DATA_TEST_CASE(hosts_x_threads,
    (boost::unit_test::data::xrange(1, 16) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0, 2>(hosts_num)},
            {"b", actions::remember, hosts_count_from_num<2, 2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

BOOST_DATA_TEST_CASE(hosts_x_threads_reverse,
    (boost::unit_test::data::xrange(1, 16) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0, 2>(hosts_num)},
            {"b", actions::remember, hosts_count_from_num<2, 2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test(start_order::node_host_reverse);
}


BOOST_DATA_TEST_CASE(chain_cancellation_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 4) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count},
            {"b", actions::remember, hosts_count},
        }
    }
    .threads(threads_count)
    .test_cancellation();
}


BOOST_DATA_TEST_CASE(chain_immediate_cancellation_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 4) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count},
            {"b", actions::remember, hosts_count},
        }
    }
    .threads(threads_count)
    .test_immediate_cancellation();
}

BOOST_AUTO_TEST_SUITE_END()
