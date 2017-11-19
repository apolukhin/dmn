#include "netlink.hpp"
#include "node.hpp"
#include <numeric>

#include <boost/test/unit_test.hpp>
#include "tests_common.hpp"

namespace {

void netlink_back_and_forth_test_impl(dmn::packet_native_t&& packet) {
    boost::asio::ip::tcp::acceptor  acceptor{
        dmn::node_t::ios(),
        boost::asio::ip::tcp::endpoint(
            boost::asio::ip::tcp::v4(), 63101  // TODO: port
        )
    };
    boost::asio::ip::tcp::socket    new_socket{dmn::node_t::ios()};
    const dmn::packet_native_t ethalon = tests::clone(packet);
    dmn::netlink_read_ptr           netlink_in;

    bool sended = false;
    int received = 0;
    bool accepted = false;

    acceptor.async_accept(new_socket, [&](const boost::system::error_code& error) {
        BOOST_TEST(!error);
        accepted = true;

        netlink_in = dmn::netlink_read_t::construct(
            std::move(new_socket),
            [&](auto*, auto& e){
                BOOST_TEST(received == 1);
            },
            [&](dmn::packet_native_t&& packet) {
                BOOST_CHECK(!ethalon.body_.data_.empty());
                BOOST_CHECK(packet.body_.data_ == ethalon.body_.data_);
                ++ received;
            }
        );
    });


    dmn::netlink_write_ptr netlink_out = dmn::netlink_write_t::construct("127.0.0.1", 63101, dmn::node_t::ios(),
        [&](dmn::netlink_write_t* l, const boost::system::error_code&, auto&& g) {
            BOOST_TEST(l == netlink_out.get());
            l->async_connect(std::move(g));
        },
        [&](dmn::netlink_write_t* l, auto g) {
            if (sended) return;
            sended = true;
            netlink_out->async_send(std::move(g), std::move(packet));
        }
    );

    dmn::node_t::ios().reset();
    dmn::node_t::ios().poll();

    netlink_in.reset();
    netlink_out.reset();

    dmn::node_t::ios().poll();

    BOOST_CHECK(sended);
    BOOST_CHECK(accepted);
    BOOST_CHECK(received == 1);
}

}

BOOST_AUTO_TEST_CASE(netlink_back_and_forth) {
    dmn::packet_native_t native1;
    const unsigned char d1[] = "hello";
    native1.body_.add_data(d1, 5, "type1");
    native1.body_.add_data(d1+2, 3, "type2");

    netlink_back_and_forth_test_impl(std::move(native1));
}


BOOST_AUTO_TEST_CASE(netlink_back_and_forth_huge_data) {
    dmn::packet_native_t p;
    std::vector<unsigned char> data;
    data.resize(1024*1024*16, 'X');
    p.body_.add_data(&data.front(), data.size(), "type_huge");

    netlink_back_and_forth_test_impl(std::move(p));
}

BOOST_AUTO_TEST_CASE(netlink_back_and_forth_empty) {
    dmn::packet_native_t p;
    p.body_.add_data(nullptr, 0, "");
    netlink_back_and_forth_test_impl(std::move(p));
}

BOOST_AUTO_TEST_CASE(netlink_back_and_forth_multiple_flags) {
    dmn::packet_native_t p;

    char name[2] = {0, 0};
    for (unsigned i = 0; i < 256; ++i) {
        ++name[0];
        p.body_.add_data(nullptr, 0, name);
    }
    netlink_back_and_forth_test_impl(std::move(p));

    new int;
}

