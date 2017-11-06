#include "packet.hpp"
#include "packet_network.hpp"
#include <numeric>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(packet_headers_conversions) {
    dmn::packet_header_native_t native1;

    unsigned char* native1_ptr = reinterpret_cast<unsigned char*>(&native1);
    std::iota(native1_ptr, native1_ptr + sizeof(dmn::packet_network_t), 1);

    dmn::packet_header_native_t native2 = dmn::packet_header_network_t(dmn::packet_header_native_t{native1}).to_native();

    const bool res = !std::memcmp(&native1, &native2, sizeof(native2));
    BOOST_CHECK(res);
}


BOOST_AUTO_TEST_CASE(packet_empty_body_conversions) {
    dmn::packet_body_native_t native1;
    dmn::packet_body_native_t native2 = dmn::packet_body_network_t(dmn::packet_body_native_t{native1}).to_native();
    BOOST_CHECK(native1.data_ == native2.data_);
}

BOOST_AUTO_TEST_CASE(packet_nonempty_body_conversions) {
    dmn::packet_body_native_t native1;

    const unsigned char d1[] = "hello";
    native1.add_data(d1, 5, "type1");
    native1.add_data(d1+2, 3, "type2");

    const dmn::packet_body_native_t native2 = dmn::packet_body_network_t(dmn::packet_body_native_t{native1}).to_native();
    BOOST_CHECK(native1.data_ == native2.data_);
}
