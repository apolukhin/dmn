#include "nodes_tester.hpp"

BOOST_AUTO_TEST_SUITE(read_n_write_n)

BOOST_DATA_TEST_CASE(hosts_x_threads,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b0 -> c; a -> b1 -> c;"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num)},
            {"b0", actions::resend, 1 /*hosts_count_from_num<1>(hosts_num)*/},
            {"b1", actions::resend, 1 /*hosts_count_from_num<1>(hosts_num)*/},
            {"c", actions::remember, hosts_count_from_num<2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

/*
BOOST_DATA_TEST_CASE(node_start_permutations,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(0, (int)tests::start_order::end_)),
    hosts_num, threads_count, start_order_int
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num)},
            {"b", actions::resend, hosts_count_from_num<1>(hosts_num)},
            {"c", actions::remember, hosts_count_from_num<2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test(static_cast<tests::start_order>(start_order_int));
}

BOOST_DATA_TEST_CASE(producer_hosts_disbalanced_x_threads,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num) + 25},
            {"b", actions::resend, hosts_count_from_num<1>(hosts_num)},
            {"c", actions::remember, hosts_count_from_num<2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

BOOST_DATA_TEST_CASE(middle_hosts_disbalanced_x_threads,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num)},
            {"b", actions::resend, hosts_count_from_num<1>(hosts_num) + 25},
            {"c", actions::remember, hosts_count_from_num<2>(hosts_num)},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

BOOST_DATA_TEST_CASE(consumer_hosts_disbalanced_x_threads,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num)},
            {"b", actions::resend, hosts_count_from_num<1>(hosts_num)},
            {"c", actions::remember, hosts_count_from_num<2>(hosts_num) + 25},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

BOOST_DATA_TEST_CASE(consumer_and_producer_hosts_disbalanced_x_threads,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num) + 25},
            {"b", actions::resend, hosts_count_from_num<1>(hosts_num)},
            {"c", actions::remember, hosts_count_from_num<2>(hosts_num) + 25},
        }
    }
    .threads(threads_count)
    .sequence_max(256)
    .test();
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// Tests where only some nodes were started or some of the nodes survived
///


BOOST_DATA_TEST_CASE(some_nodes_failed_to_start,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num) + 1},
            {"b", actions::remember, hosts_count_from_num<1>(hosts_num) + 1},
            {"c", actions::resend, hosts_count_from_num<2>(hosts_num) + 1},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},
        {"b", 1},
        {"c", 0},
    })
    .test();
}

BOOST_DATA_TEST_CASE(some_nodes_survived,
    (boost::unit_test::data::xrange(1, 8) * boost::unit_test::data::xrange(1, 5)),
    hosts_num, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, hosts_count_from_num<0>(hosts_num) + 1},
            {"b", actions::remember, hosts_count_from_num<1>(hosts_num) + 1},
            {"c", actions::resend, hosts_count_from_num<2>(hosts_num) + 1},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},
        {"b", 1},
        {"c", 0},
    })
    .test_death(50_perc);
}

BOOST_DATA_TEST_CASE(few_producers_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
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
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
    })
    .test_death(10_perc);
}

BOOST_DATA_TEST_CASE(few_middles_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_middles_survived,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
    })
    .test_death(10_perc);
}

BOOST_DATA_TEST_CASE(few_consumers_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"c", 0},{"c", 1},{"c", 2},{"c", 3},{"c", 4},{"c", 5},{"c", 6},{"c", 7},{"c", 8},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_consumers_survived,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"c", 0},{"c", 1},{"c", 2},{"c", 3},{"c", 4},{"c", 5},{"c", 6},{"c", 7},{"c", 8},
    })
    .test_death(9_perc);
}

BOOST_DATA_TEST_CASE(few_nodes_started,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
        {"c", 0},{"c", 1},{"c", 2},{"c", 3},{"c", 4},{"c", 5},{"c", 6},{"c", 7},{"c", 8},
    })
    .test();
}

BOOST_DATA_TEST_CASE(few_nodes_survived,
    boost::unit_test::data::xrange(1, 5),
    threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b -> c"},
        {
            {"a", actions::generate, 10},
            {"b", actions::remember, 10},
            {"c", actions::resend, 10},
        }
    }
    .threads(threads_count)
    .sequence_max(512)
    .skip({
        {"a", 0},{"a", 1},{"a", 2},{"a", 3},{"a", 4},{"a", 5},{"a", 6},{"a", 7},{"a", 8},
        {"b", 0},{"b", 1},{"b", 2},{"b", 3},{"b", 4},{"b", 5},{"b", 6},{"b", 7},{"b", 8},
        {"c", 0},{"c", 1},{"c", 2},{"c", 3},{"c", 4},{"c", 5},{"c", 6},{"c", 7},{"c", 8},
    })
    .test_death(9_perc);
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// Tests long chain is canceled
///


BOOST_DATA_TEST_CASE(chain_cancellation_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b0 -> b1 -> b2 -> b3 -> b4 -> c"},
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
    .test_cancellation();
}


BOOST_DATA_TEST_CASE(chain_immediate_cancellation_hosts_x_threads,
    (boost::unit_test::data::xrange(1, 5) * boost::unit_test::data::xrange(1, 5)),
    hosts_count, threads_count
) {
    nodes_tester_t{
        tests::links_t{"a -> b0 -> b1 -> b2 -> b3 -> b4 -> c"},
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
    .test_immediate_cancellation();
}
*/
BOOST_AUTO_TEST_SUITE_END()
