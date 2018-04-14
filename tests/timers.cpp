#include "impl/net/interval_timer.hpp"
#include <boost/system/error_code.hpp>
#include <thread>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

BOOST_AUTO_TEST_SUITE(timers)

BOOST_DATA_TEST_CASE(multiple_waiters,
    (boost::unit_test::data::xrange(1, 16)),
    waiters_count
) {
    boost::asio::io_context ios;
    dmn::interval_timer t{ios, std::chrono::milliseconds(15)};

    int incs = 0;
    for (int i = 0; i < waiters_count; ++i) {
        t.async_wait_err([&incs](auto ec) {
            BOOST_TEST(!ec);
            ++ incs;
        });
    }

    ios.run_for(std::chrono::milliseconds(25));
    ios.stop();
    t.close();
    BOOST_TEST(incs == waiters_count);
}


BOOST_DATA_TEST_CASE(multiple_waiters_twice,
    (boost::unit_test::data::xrange(1, 16)),
    waiters_count
) {
    boost::asio::io_context ios;
    dmn::interval_timer t{ios, std::chrono::milliseconds(15)};

    int incs = 0;
    for (int i = 0; i < waiters_count; ++i) {
        t.async_wait_err([&incs, &t](auto ec) {
            BOOST_TEST(!ec);
            ++ incs;

            t.async_wait_err([&incs](auto ec) {
                BOOST_TEST(!ec);
                ++ incs;
            });
        });
    }

    ios.run_for(std::chrono::milliseconds(45));
    ios.stop();
    t.close();
    BOOST_TEST(incs == waiters_count * 2);
}

BOOST_DATA_TEST_CASE(multiple_waiters_cancel,
    (boost::unit_test::data::xrange(1, 16)),
    waiters_count
) {
    boost::asio::io_context ios;
    dmn::interval_timer t{ios, std::chrono::milliseconds(15)};

    int incs = 0;
    for (int i = 0; i < waiters_count; ++i) {
        t.async_wait_err([&incs, &t](auto ec) {
            BOOST_TEST(!ec);
            ++ incs;

            t.async_wait_err([&incs](auto ec) {
                BOOST_TEST(!ec);
                ++ incs;
            });
        });
    }

    ios.run_for(std::chrono::milliseconds(20));
    ios.stop();
    t.close();
    BOOST_TEST(incs == waiters_count);
}

BOOST_DATA_TEST_CASE(multiple_waiters_async_addition,
    (boost::unit_test::data::xrange(50, 60)),
    waiters_count
) {
    boost::asio::io_context ios;
    dmn::interval_timer t{ios, std::chrono::milliseconds(1)};

    std::thread thr{[&]{
        for (int i = 0; i < waiters_count; ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(777));
            t.async_wait_err([&t](auto ec) {
                BOOST_TEST((!ec || ec == boost::system::errc::operation_canceled));

                t.async_wait_err([](auto ec) {
                    BOOST_TEST((!ec || ec == boost::system::errc::operation_canceled));
                });
            });
        }
    }};


    ios.run_for(std::chrono::milliseconds(20));
    ios.stop();
    thr.join();

    t.close();
}

BOOST_DATA_TEST_CASE(multiple_waiters_async_addition_no_errc,
    (boost::unit_test::data::xrange(50, 60)),
    waiters_count
) {
    boost::asio::io_context ios;
    dmn::interval_timer t{ios, std::chrono::milliseconds(1)};

    std::thread thr{[&]{
        for (int i = 0; i < waiters_count; ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(777));
            t.async_wait([&t]() {
                t.async_wait([](){});
            });
        }
    }};

    ios.run_for(std::chrono::milliseconds(20));
    ios.stop();
    thr.join();

    t.close();

    BOOST_TEST(true); // silencing Boost.Test warning `did not check any assertions`
}

BOOST_AUTO_TEST_SUITE_END()
