#include "load_graph.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(load_graph)

struct exception_message {
    const std::string msg;

    template <class T>
    bool operator()(const T& e) const {
        BOOST_TEST(e.what() == msg);
        return e.what() == msg;
    }
};

BOOST_AUTO_TEST_CASE(wrong_syntax1) {
    const std::string ss{R"(
        digrph test
        {
        }
    )"};
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::exception, exception_message{"Wanted \"graph\" or \"digraph\" (token is \"<identifier> 'digrph'\")"});
}

BOOST_AUTO_TEST_CASE(wrong_syntax2) {
    const std::string ss{R"(
        digraph test
        { a -----> b
        }
    )"};
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::exception, exception_message{"Using -- in directed graph (token is \"<dash-dash> '--'\")"});
}

BOOST_AUTO_TEST_CASE(base) {
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

    const std::string ss4_long0(R"(
        digraph test
        {
           a -> b;
           b -> c;
           c -> d;
           d -> e;
           e -> f;
           a -> b0;
           b0 -> b1;
           b1 -> b2;
           b2 -> b3;
           b3 -> b4;
           b4 -> b5;
           b5 -> b0;
           b5 -> f;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss4_long0), std::runtime_error, exception_message{"Graph has a circut: b0 -> b1 -> b2 -> b3 -> b4 -> b5 -> b0"});

    const std::string ss4_long1(R"(
        digraph test
        {
           a -> b;
           b -> c;
           a -> b0;
           b0 -> b1;
           b1 -> b2;
           b2 -> b3;
           b3 -> b4;
           b4 -> b5;
           b5 -> b0;
           b5 -> f;
           c -> d;
           d -> e;
           e -> f;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss4_long1), std::runtime_error, exception_message{"Graph has a circut: b0 -> b1 -> b2 -> b3 -> b4 -> b5 -> b0"});

    const std::string ss4_long2(R"(
        digraph test
        {
           a -> b;
           b -> c;
           a -> b0;
           b0 -> b1;
           b4 -> b5;
           b5 -> b0;
           b5 -> f;
           c -> d;
           d -> e;
           e -> f;
           b1 -> b2;
           b2 -> b3;
           b3 -> b4;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss4_long2), std::runtime_error, exception_message{"Graph has a circut: b0 -> b1 -> b2 -> b3 -> b4 -> b5 -> b0"});


    const std::string ss4_long3(R"(
        digraph test
        {
           a -> b;
           b -> c;
           a -> b0;
           b0 -> b1;
           b1 -> f;
           b4 -> b5;
           b5 -> b0;
           b5 -> f;
           c -> d;
           d -> e;
           e -> f;
           b1 -> b2;
           b2 -> b3;
           b3 -> b4;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss4_long3), std::runtime_error, exception_message{"Graph has a circut: b0 -> b1 -> b2 -> b3 -> b4 -> b5 -> b0"});

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
          "Vertex 'a' and all other vertexes must have non empty 'hosts' property. Example:\n"
          "(digraph example {\n"
          "    a [hosts = \"127.0.0.1:44001\"];\n"
          "    b [hosts = \"127.0.0.1:44003\"];\n"
          "    a -> b;\n"
          "}"
    });
}

BOOST_AUTO_TEST_CASE(graph_vertex_data_hosts_validation) {
    const std::string ss(R"(
        digraph test
        {
            a [hosts = "127.0.0.1:44001"];
            b [hosts = "127.0.0.1:44001"];
            a -> b;
        }
    )");
    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message {
        "Same host:port for vertexes 'a' and 'b'"
    });
}

BOOST_AUTO_TEST_CASE(graph_vertex_data_too_many_outgoing_edges) {
    std::string ss;
    ss.reserve(20 * std::numeric_limits<std::uint16_t>::max());
    ss += "digraph test {";

    for (std::size_t i = 0; i <= dmn::max_in_or_out_edges_per_node; ++i) {
        const auto id = boost::lexical_cast<std::array<char, 30>>(i);
        ss += "a->b";
        ss += id.data();
        ss += ";b";
        ss += id.data();
        ss += "->c;";
    }
    ss += "}";

    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message {
        "Each vertex must have at most 11000 outgoing edges. Vertex 'a' has 11001 outgoing edges."
    });
}

BOOST_AUTO_TEST_CASE(graph_vertex_data_too_many_incomming_edges) {
    std::string ss;
    ss.reserve(20 * std::numeric_limits<std::uint16_t>::max());
    ss += "digraph test { c;";

    for (std::size_t i = 0; i < dmn::max_in_or_out_edges_per_node - 1; ++i) {
        const auto id = boost::lexical_cast<std::array<char, 30>>(i);
        ss += "a->b";
        ss += id.data();
        ss += ";b";
        ss += id.data();
        ss += "->c;";
    }
    ss += "a->z;";
    ss += "z->x0;x0->c;";
    ss += "z->x1;x1->c;";
    ss += "z->x2;x2->c;";
    ss += "z->x3;x3->c;";
    ss += "z->x4;x4->c;";
    ss += "}";

    BOOST_CHECK_EXCEPTION(dmn::load_graph(ss), std::runtime_error, exception_message {
        "Each vertex must have at most 11000 incomming edges. Vertex 'c' has 11004 incomming edges."
    });
}

BOOST_AUTO_TEST_SUITE_END()
