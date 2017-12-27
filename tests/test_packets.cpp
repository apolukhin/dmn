#include "packet.hpp"
#include "packet_network.hpp"
#include <numeric>

#include <boost/test/unit_test.hpp>
#include "tests_common.hpp"


BOOST_AUTO_TEST_CASE(packet_empty_body_conversions) {
    dmn::packet_t native1;
    dmn::packet_t native2 {
        dmn::packet_network_t(dmn::packet_t{}).to_native()
    };
    BOOST_CHECK(native1.raw_storage() == native2.raw_storage());
}

BOOST_AUTO_TEST_CASE(packet_nonempty_body_conversions) {
    dmn::packet_t native1;

    const unsigned char d1[] = "hello";
    native1.add_data(d1, 5, "type1");
    native1.add_data(d1+2, 3, "type2");

    const dmn::packet_t native2 {
        dmn::packet_network_t(tests::clone(native1)).to_native()
    };
    BOOST_CHECK(native1.raw_storage() == native2.raw_storage());
}
