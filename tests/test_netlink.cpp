#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/net/tcp_acceptor.hpp"
#include "impl/net/tcp_read_proto.hpp"
#include "impl/net/tcp_write_proto.hpp"
#include <numeric>

#include <boost/test/unit_test.hpp>
#include "tests_common.hpp"

BOOST_AUTO_TEST_SUITE(netlink)

namespace {

void netlink_back_and_forth_test_impl(dmn::packet_t&& packet) {
    boost::asio::io_context ios;

    dmn::tcp_acceptor acceptor{ios, "127.0.0.1", 63101};
    const dmn::packet_t ethalon = tests::clone(packet);
    dmn::packet_network_t packet_network{std::move(packet)};

    using netlink_in_t = dmn::netlink_t<std::vector<unsigned char>, dmn::tcp_read_proto_t>;
    std::unique_ptr<netlink_in_t> netlink_in;


    bool sended = false;
    int received = 0;
    bool accepted = false;

    acceptor.async_accept([&](const boost::system::error_code& error) {
        BOOST_TEST(!error);
        accepted = true;

        netlink_in = netlink_in_t::construct(
            acceptor.extract_socket(),
            [&](auto& /*proto*/, auto& e){
                BOOST_TEST(received == 1);
            },
            [&](auto& /*proto*/) {
                BOOST_CHECK(!ethalon.raw_storage().empty());
                BOOST_CHECK(netlink_in->packet == ethalon.raw_storage());
                ++ received;
            }
        );

        acceptor.close();

        netlink_in->packet.resize(boost::asio::buffer_size(packet_network.const_buffer()));

        netlink_in->async_read(boost::asio::buffer(netlink_in->packet));
    });

    using netlink_out_t = dmn::netlink_t<int, dmn::tcp_write_proto_t>;
    std::unique_ptr<netlink_out_t> netlink_out = netlink_out_t::construct("127.0.0.1", 63101, ios,
        [&](const boost::system::error_code&, auto g, dmn::tcp_write_proto_t::send_error_tag) {
            BOOST_TEST(static_cast<bool>(g));
            BOOST_TEST(false);
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

    ios.reset();
    ios.poll();

    netlink_in->close();
    netlink_in.reset();
    netlink_out->close();
    netlink_out.reset();


    BOOST_CHECK(sended);
    BOOST_CHECK(accepted);
    BOOST_CHECK(received == 1);
}

}

BOOST_AUTO_TEST_CASE(back_and_forth) {
    dmn::packet_t native1;
    const unsigned char d1[] = "hello";
    native1.add_data(d1, 5, "type1");
    native1.add_data(d1+2, 3, "type2");

    netlink_back_and_forth_test_impl(std::move(native1));
}


BOOST_AUTO_TEST_CASE(back_and_forth_huge_data) {
    dmn::packet_t p;
    std::vector<unsigned char> data;
    data.resize(1024*1024*16, 'X');
    p.add_data(&data.front(), data.size(), "type_huge");

    netlink_back_and_forth_test_impl(std::move(p));
}

BOOST_AUTO_TEST_CASE(back_and_forth_empty) {
    dmn::packet_t p;
    p.add_data(nullptr, 0, "");
    netlink_back_and_forth_test_impl(std::move(p));
}

BOOST_AUTO_TEST_CASE(back_and_forth_multiple_flags) {
    dmn::packet_t p;

    char name[2] = {0, 0};
    for (unsigned i = 0; i < 256; ++i) {
        ++name[0];
        p.add_data(nullptr, 0, name);
    }
    netlink_back_and_forth_test_impl(std::move(p));
}

// Tries to reconnect untill succeeds

struct socket_reconnect_t {
    const char* name;
    boost::asio::ip::tcp::tcp::socket socket;
    const boost::asio::ip::tcp::endpoint ep;
    bool started = false;
    bool success = false;

    void operator()(boost::system::error_code ec) {
        if (ec) {
            BOOST_TEST(!success);
            socket.async_connect(ep,
                [this](auto ec){ (*this)(ec); }
            );
            return;
        }
        success = true;
        boost::system::error_code ignore;
        socket.shutdown(boost::asio::socket_base::shutdown_both, ignore);
        socket.close();
    }

    void operator()() {
        if (started) {
            return;
        }
        started = true;
        socket.async_connect(ep,
            [this](auto ec){ (*this)(ec); }
        );
    }
};

BOOST_AUTO_TEST_CASE(acceptor_rebind) {
    boost::asio::io_context ios;

    const boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address::from_string("127.0.0.1"), 42042};
    socket_reconnect_t socket1{"1 socket", boost::asio::ip::tcp::tcp::socket{ios}, ep};
    socket_reconnect_t socket2{"2 socket", boost::asio::ip::tcp::tcp::socket{ios}, ep};

    dmn::tcp_acceptor accept1{ios, "127.0.0.1", 42042};
    boost::optional<dmn::tcp_acceptor> accept2;

    accept1.async_accept([&](auto /*ec*/){
        accept1.close();
        accept2.emplace(ios, "127.0.0.1", 42042);
        accept2->async_accept([&](auto /*ec*/){
            accept2->close();
        });

        socket2();
    });

    socket1();
    ios.run();

    BOOST_TEST(socket1.started);
    BOOST_TEST(socket1.success); // socket reconnected
    BOOST_TEST(socket2.started);
    BOOST_TEST(socket2.success); // socket reconnected
}

BOOST_AUTO_TEST_SUITE_END()
