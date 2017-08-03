#include "load_graph.hpp"

#include <boost/test/unit_test.hpp>

#include <sstream>

struct exception_message {
    const std::string msg;

    template <class T>
    bool operator()(const T& e) const {
        BOOST_TEST(e.what() == msg);
        return e.what() == msg;
    }
};

BOOST_AUTO_TEST_CASE(graph_loading) {
    std::stringstream ss{R"(
        digraph graph
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
    std::stringstream ss;
    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> c;
           c -> b;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has no sink! There must be one vertex with incomming edges and no outgoing edges."});


    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> a;
           b -> c;
           a -> c;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has no source! There must be one vertex with outgoing edges and no incomming edges."});


    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> c;
           c -> b;
           c -> d;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has a circut: b -> c -> b"});


    ss.str(R"(
        digraph graph
        {
           a -> b;
           b -> c;
           d -> e;
           e -> d;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has a circut: d -> e -> d"});


    ss.str(R"(
        digraph graph
        {
           a;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has a dangling vertex 'a'"});


    ss.str(R"(
        digraph graph
        {
           a0 -> a1;
           a -> a;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has a circut: a -> a"});


    ss.str(R"(
        digraph graph
        {
           a0 -> a -> a1;
           a -> a;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message{"Graph has a circut: a -> a"});
}


BOOST_AUTO_TEST_CASE(graph_vertex_data_validation) {
    std::stringstream ss;
    ss.str(R"(
        digraph graph
        {
           a -> b;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message {
        "Each vertex must have a non empty hosts property. Example:\n"
        "(digraph graph {\n"
        "    a [hosts = '127.0.0.1:44001'];\n"
        "    b [hosts = '127.0.0.1:44003'];\n"
        "    a -> b;\n"
        "}"
    });
}
