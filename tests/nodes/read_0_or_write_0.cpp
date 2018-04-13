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

BOOST_DATA_TEST_CASE(node_start_permutations,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(0, (int)tests::start_order::end_)),
    hosts_num, threads_count, start_order_int
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num)},
            {"b", actions::remember, hosts_count_from_num<2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test(static_cast<tests::start_order>(start_order_int));
}

BOOST_DATA_TEST_CASE(producer_hosts_disbalanced_x_threads,
    (boost::unit_test::data::xrange(1, 16) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0, 2>(hosts_num) + 25},
            {"b", actions::remember, hosts_count_from_num<2, 2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

BOOST_DATA_TEST_CASE(consumer_hosts_disbalanced_x_threads,
    (boost::unit_test::data::xrange(1, 16) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0, 2>(hosts_num)},
            {"b", actions::remember, hosts_count_from_num<2, 2>(hosts_num) + 25},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
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

///////////////////////////////////////////////////////////////////////////////////////////////
/// Tests where only some nodes were started or some of the nodes survived
///

BOOST_DATA_TEST_CASE(few_nodes_failed_to_start,
    (boost::unit_test::data::xrange(1, 16) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0, 2>(hosts_num) + 1},
            {"b", actions::remember, hosts_count_from_num<2, 2>(hosts_num) + 1},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},
        {"b", 1},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_nodes_died,
    (boost::unit_test::data::xrange(1, 16) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, hosts_count_from_num<0, 2>(hosts_num) + 1},
            {"b", actions::remember, hosts_count_from_num<2, 2>(hosts_num) + 1},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},
        {"b", 1},
    })
    .test_death(ethalon_match::partial);
}

BOOST_DATA_TEST_CASE(few_producers_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_producers_survived,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
    })
    .test_death(ethalon_match::partial);
}

BOOST_DATA_TEST_CASE(few_consumers_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_consumers_survived,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
    })
    .test_death(ethalon_match::partial);
}

BOOST_DATA_TEST_CASE(few_nodes_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_nodes_survived,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
    })
    .test_death(ethalon_match::partial);
}

BOOST_AUTO_TEST_SUITE_END()
