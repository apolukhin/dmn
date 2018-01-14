#include "impl/packet.hpp"
#include "impl/net/packet_network.hpp"
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

BOOST_AUTO_TEST_CASE(packet_set_get_small_type_name) {
    dmn::packet_t native;

    const unsigned char d[] = "Hello";
    native.add_data(d, sizeof(d), "some type");
    BOOST_TEST(reinterpret_cast<const char*>(native.get_data("some type").first) == std::string("Hello"));
    BOOST_TEST(native.get_data("some type").second == sizeof(d));
}


BOOST_AUTO_TEST_CASE(packet_set_get_huge_type_name) {
    dmn::packet_t native;

    std::string huge_type_name(65535 + 100, 'c');
    BOOST_TEST(huge_type_name.size() > 65535);

    const unsigned char d[] = "Hello";
    native.add_data(d, sizeof(d), huge_type_name.c_str());
    BOOST_TEST(reinterpret_cast<const char*>(native.get_data(huge_type_name.c_str()).first) == std::string("Hello"));
    BOOST_TEST(native.get_data(huge_type_name.c_str()).second == sizeof(d));
}

BOOST_AUTO_TEST_CASE(packet_merge_basic) {
    dmn::packet_t native1;

    const unsigned char d1[] = "hello";
    native1.add_data(d1, 6, "type1");
    native1.add_data(d1+2, 4, "type2");

    dmn::packet_t native2;
    native2.add_data(d1, 6, "type3");
    native2.add_data(d1+2, 4, "type4");

    native1.header().wave_id = static_cast<dmn::wave_id_t>(0xFF00FF00);
    native2.header().wave_id = native1.header().wave_id;

    dmn::packet_network_t net1(std::move(native1));
    dmn::packet_network_t net2(std::move(native2));

    net1.merge_packet(std::move(net2));

    const dmn::packet_t result = std::move(net1).to_native();

    BOOST_TEST(result.get_data("type1").second == sizeof(d1));
    BOOST_TEST(result.get_data("type2").second == 4);
    BOOST_TEST(result.get_data("type3").second == sizeof(d1));
    BOOST_TEST(result.get_data("type4").second == 4);

    BOOST_TEST(reinterpret_cast<const char*>(result.get_data("type1").first) == std::string("hello"));
    BOOST_TEST(reinterpret_cast<const char*>(result.get_data("type2").first) == std::string("llo"));
    BOOST_TEST(reinterpret_cast<const char*>(result.get_data("type3").first) == std::string("hello"));
    BOOST_TEST(reinterpret_cast<const char*>(result.get_data("type4").first) == std::string("llo"));
}

// TODO: tests for data types deduplication on add_data

