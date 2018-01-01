#include "load_graph.hpp"

#include <boost/test/unit_test.hpp>

struct exception_message {
    const std::string msg;

    template <class T>
    bool operator()(const T& e) const {
        BOOST_TEST(e.what() == msg);
        return e.what() == msg;
    }
};

BOOST_AUTO_TEST_CASE(graph_loading_base) {
    const std::string ss{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44003"];
            a -> b;
        }
    )"};
    BOOST_TEST(boost::num_vertices(dmn::load_graph(ss)) == 2);

}

BOOST_AUTO_TEST_CASE(graph_loading) {
    const std::string ss{R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            c [hosts = "127.0.0.1:44002"];
            b [hosts = "127.0.0.1:44003"];
            a -> b;
            b -> c;
        }
    )"};
    BOOST_TEST(boost::num_vertices(dmn::load_graph(ss)) == 3);

}

BOOST_AUTO_TEST_CASE(graph_topology_validation) {
    const std::string ss1(R"(
        digraph test
        {
           a -> b;
           b -> c;
           c -> b;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss1), std::runtime_error, exception_message{"Graph has no sink! There must be one vertex with incomming edges and no outgoing edges."});


    const std::string ss2(R"(
        digraph test
        {
           a -> b;
           b -> a;
           b -> c;
           a -> c;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss2), std::runtime_error, exception_message{"Graph has no source! There must be one vertex with outgoing edges and no incomming edges."});


    const std::string ss3(R"(
        digraph test
        {
           a -> b;
           b -> c;
           c -> b;
           c -> d;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss3), std::runtime_error, exception_message{"Graph has a circut: b -> c -> b"});


    const std::string ss4(R"(
        digraph test
        {
           a -> b;
           b -> c;
           d -> e;
           e -> d;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss4), std::runtime_error, exception_message{"Graph has a circut: d -> e -> d"});


    const std::string ss5(R"(
        digraph test
        {
           a;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss5), std::runtime_error, exception_message{"Graph has a dangling vertex 'a'"});


    const std::string ss6(R"(
        digraph test
        {
           a0 -> a1;
           a -> a;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss6), std::runtime_error, exception_message{"Graph has a circut: a -> a"});


    const std::string ss7(R"(
        digraph test
        {
           a0 -> a -> a1;
           a -> a;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss7), std::runtime_error, exception_message{"Graph has a circut: a -> a"});
}


BOOST_AUTO_TEST_CASE(graph_vertex_data_validation) {
    const std::string ss(R"(
        digraph test
        {
           a -> b;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message {
        "Each vertex must have a non empty hosts property. Example:\n"
        "(digraph example {\n"
        "    a [hosts = '127.0.0.1:44001'];\n"
        "    b [hosts = '127.0.0.1:44003'];\n"
        "    a -> b;\n"
        "}"
    });
}
