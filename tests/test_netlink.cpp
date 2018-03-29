#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/net/tcp_read_proto.hpp"
#include "impl/net/tcp_write_proto.hpp"
#include <numeric>

#include <boost/test/unit_test.hpp>
#include "tests_common.hpp"

namespace {

void netlink_back_and_forth_test_impl(dmn::packet_t&& packet) {
    boost::asio::ip::tcp::acceptor  acceptor{
        dmn::node_t::ios(),
        boost::asio::ip::tcp::endpoint(
            boost::asio::ip::tcp::v4(), 63101  // TODO: port
        )
    };
    boost::asio::ip::tcp::socket    new_socket{dmn::node_t::ios()};
    const dmn::packet_t ethalon = tests::clone(packet);
    dmn::packet_network_t packet_network{std::move(packet)};

    using netlink_in_t = dmn::netlink_t<std::vector<unsigned char>, dmn::tcp_read_proto_t>;
    std::unique_ptr<netlink_in_t> netlink_in;


    bool sended = false;
    int received = 0;
    bool accepted = false;

    acceptor.async_accept(new_socket, [&](const boost::system::error_code& error) {
        BOOST_TEST(!error);
        accepted = true;

        netlink_in = netlink_in_t::construct(
            std::move(new_socket),
            [&](auto& /*proto*/, auto& e){
                BOOST_TEST(received == 1);
            },
            [&](auto& /*proto*/) {
                BOOST_CHECK(!ethalon.raw_storage().empty());
                BOOST_CHECK(netlink_in->packet == ethalon.raw_storage());
                ++ received;
            }
        );
        netlink_in->packet.resize(boost::asio::buffer_size(packet_network.const_buffer()));

        netlink_in->async_read(boost::asio::buffer(netlink_in->packet));
    });

    using netlink_out_t = dmn::netlink_t<int, dmn::tcp_write_proto_t>;
    std::unique_ptr<netlink_out_t> netlink_out = netlink_out_t::construct("127.0.0.1", 63101, dmn::node_t::ios(),
        [&](const boost::system::error_code&, auto g, dmn::tcp_write_proto_t::send_error_tag) {
            BOOST_TEST(static_cast<bool>(g));
            BOOST_TEST(false);
            /*auto* l = g.mutex();
            BOOST_TEST(l == netlink_out.get());
            l->async_reconnect(std::move(g));*/
        },
        [&](auto g) {
            if (sended) return;
            sended = true;
            netlink_out->async_send(std::move(g), packet_network.const_buffer());
        },
        [&](const boost::system::error_code&, auto g, dmn::tcp_write_proto_t::reconnect_error_tag) {
            BOOST_TEST(static_cast<bool>(g));
            auto* l = g.mutex();
            BOOST_TEST(l == netlink_out.get());
            l->async_reconnect(std::move(g));
        }
    );
    netlink_out->async_reconnect(netlink_out->try_lock());

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
    dmn::packet_t native1;
    const unsigned char d1[] = "hello";
    native1.add_data(d1, 5, "type1");
    native1.add_data(d1+2, 3, "type2");

    netlink_back_and_forth_test_impl(std::move(native1));
}


BOOST_AUTO_TEST_CASE(netlink_back_and_forth_huge_data) {
    dmn::packet_t p;
    std::vector<unsigned char> data;
    data.resize(1024*1024*16, 'X');
    p.add_data(&data.front(), data.size(), "type_huge");

    netlink_back_and_forth_test_impl(std::move(p));
}

BOOST_AUTO_TEST_CASE(netlink_back_and_forth_empty) {
    dmn::packet_t p;
    p.add_data(nullptr, 0, "");
    netlink_back_and_forth_test_impl(std::move(p));
}

BOOST_AUTO_TEST_CASE(netlink_back_and_forth_multiple_flags) {
    dmn::packet_t p;

    char name[2] = {0, 0};
    for (unsigned i = 0; i < 256; ++i) {
        ++name[0];
        p.add_data(nullptr, 0, name);
    }
    netlink_back_and_forth_test_impl(std::move(p));

    new int;
}

