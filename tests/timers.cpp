#include "impl/net/interval_timer.hpp"
#include <boost/system/error_code.hpp>
#include <thread>
#include <boost/optional.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_SUITE(timers)

BOOST_AUTO_TEST_CASE(interval_timer_simple) {
    boost::optional<boost::asio::io_context> ios;
    ios.emplace();
    int incs = 0;
    dmn::interval_timer t{*ios, std::chrono::milliseconds(15), [&incs]() {
        ++ incs;
    }};

    ios->run_for(std::chrono::milliseconds(25));
    ios->stop();
    t.close();
    ios.reset();

    BOOST_TEST(incs == 1);
}

BOOST_AUTO_TEST_CASE(interval_timer_3_times) {
    boost::optional<boost::asio::io_context> ios;
    ios.emplace();
    int incs = 0;
    dmn::interval_timer t{*ios, std::chrono::milliseconds(10), [&incs]() {
        ++ incs;
    }};


    ios->run_for(std::chrono::milliseconds(35));
    ios->stop();
    t.close();
    ios.reset();
    BOOST_TEST(incs == 3);
}


BOOST_AUTO_TEST_CASE(multiple_waiters_cancel) {
    boost::optional<boost::asio::io_context> ios;
    ios.emplace();
    int incs = 0;
    dmn::interval_timer t{*ios, std::chrono::milliseconds(100), [&incs]() {
        ++ incs;
    }};

    ios->run_for(std::chrono::milliseconds(2));
    ios->stop();
    t.close();
    ios.reset();
    BOOST_TEST(incs == 0);
}

BOOST_AUTO_TEST_SUITE_END()
