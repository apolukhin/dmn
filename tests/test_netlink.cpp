#include "netlink.hpp"
#include <numeric>

#include <boost/test/unit_test.hpp>
/*
namespace {

struct netlink_test_node: dmn::node_base_t {
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::ip::tcp::socket    new_socket_;
    dmn::netlink_read_ptr                netlink_in_;
    dmn::netlink_write_ptr                netlink_out_;

    dmn::packet_native_t            ethalon_;
    int received = 0;
    bool accepted = false;

    netlink_test_node(const dmn::graph_t& in, const char* node_id)
        : node_base_t(in, node_id)
        , acceptor_(
                ios(),
                boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(), 63101  // TODO: port
                )
            )
        , new_socket_(ios())
    {
        start_accept();
        netlink_out_
    }

    void on_accept(const boost::system::error_code& error) {
        BOOST_CHECK (!error);
        BOOST_CHECK (!netlink_);
        netlink_ = dmn::netlink_t::construct(*this, std::move(new_socket_));
    }

    void start_accept() {
        acceptor_.async_accept(new_socket_, [this](const boost::system::error_code& error) {
            on_accept(error);
        });
    }

    void stopping() override {
        acceptor_.cancel();
    }
    void stopped() override {
    }

    void on_packet_accept(dmn::packet_native_t&& packet) {
        BOOST_CHECK(!ethalon_.body_.data_.empty());
        BOOST_CHECK(packet.body_.data_ == ethalon_.body_.data_);
        -- write_work_;
        ++ received;
        if (received == 2) {
            stopping();
            return;
        }

        auto g = netlink_->try_lock();
        BOOST_CHECK(g);
        netlink_->async_send(std::move(g), std::move(packet));
    }

    void on_packet_send(dmn::packet_native_t&& packet) {
        BOOST_CHECK(false);
    }
};

void netlink_back_and_forth_test_impl(dmn::packet_native_t&& packet) {
    std::stringstream ss;
    ss.str(R"(
           digraph graph
           {
               a [hosts = "127.0.0.1"];
               b [hosts = "127.0.0.1"];
               a -> b;
           }
    )");
    netlink_test_node node(dmn::load_graph(ss), "a");

    auto link1 = dmn::netlink_t::construct(node, "127.0.0.1", 63101);
    auto guard = link1->try_lock();
    BOOST_CHECK(guard);

    node.ethalon_ = dmn::packet_native_t(packet);

    link1->async_send(std::move(guard), std::move(packet));
    node.ios().reset();
    node.ios().run();
    BOOST_CHECK(node.netlink_);
    BOOST_CHECK(node.received == 2);
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
}
*/
