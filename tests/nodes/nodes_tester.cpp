#include "nodes_tester.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <iostream>

#include "node_base.hpp"
#include "stream.hpp"

namespace tests {

namespace {
    std::mutex g_tests_mutex;

    struct nodes_guard {
        boost::asio::io_context& ios;
        std::vector<std::unique_ptr<dmn::node_base_t>>& nodes;

        ~nodes_guard() {
            ios.stop();

            for (auto& n : nodes) {
                n->single_threaded_io_detach();
            }
        }
    };
}

#define MT_BOOST_TEST(x) if (!(x)) { std::lock_guard<std::mutex> lock{g_tests_mutex}; BOOST_TEST(x); }
#define MT_BOOST_FAIL(x) { std::lock_guard<std::mutex> lock{g_tests_mutex}; BOOST_FAIL(x); }

std::map<int, unsigned> nodes_tester_t::seq_ethalon() const {
    std::map<int, unsigned> res;
    for (int i = 0; i <= max_seq_; ++i) {
        res[i] = 1;
    }
    return res;
}

void nodes_tester_t::generate_sequence(void* s_void) const {
    dmn::stream_t& s = *static_cast<dmn::stream_t*>(s_void);
    const int seq = sequence_counter_.fetch_add(1);

    if (seq <= max_seq_) {
        auto data = boost::lexical_cast<std::string>(seq);
        s.add(data.data(), data.size(), "seq");
    }

    if (seq >= max_seq_) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

void nodes_tester_t::remember_sequence(void* s_void) const {
    dmn::stream_t& s = *static_cast<dmn::stream_t*>(s_void);
    for (const char* data_type: {"seq", "seq0", "seq1", "seq2", "seq3", "seq4", "seq5", "seq6", "seq7", "seq8", "seq9", "seq10"}) {
        const auto data = s.get_data(data_type);
        if (data.second == 0) {
            continue;
        }
        unsigned seq = 0;
        const bool res = boost::conversion::try_lexical_convert<unsigned>(static_cast<const unsigned char*>(data.first), data.second, seq);
        MT_BOOST_TEST(res);
        bool shutdown = false;
        {
            std::lock_guard<std::mutex> l(seq_mutex_);
            ++sequences_[seq];
            shutdown = (sequences_ == ethalon_sequences_);
        }
        if (shutdown) {
            for (const auto& node : nodes_) {
                node->ios().stop();
            }
        }
    }
}

void nodes_tester_t::resend_sequence(void* s_void) const {
    dmn::stream_t& s = *static_cast<dmn::stream_t*>(s_void);
    const auto data = s.get_data("seq");
    s.add(data.first, data.second, "seq");
}


void nodes_tester_t::run_impl(boost::asio::io_context& ios) {
    auto ios_run = [&ios]() {
        try {
            ios.run();
        } catch (const std::exception& e) {
            MT_BOOST_FAIL("Sudden exception during run: " << e.what());
        } catch (...) {
            MT_BOOST_FAIL("Sudden unknown exception during run");
        }
    };

    threads_.reserve(threads_count_);

    for (int i = 1; i < threads_count_; ++i) {
        threads_.emplace_back(ios_run);
    }
    ios_run();
    for (auto& t: threads_) {
        t.join();
    }
    threads_.clear();
}


void nodes_tester_t::init_nodes_by(start_order order, boost::asio::io_context& ios) {
    try {
        switch (order) {
        case start_order::node_host: init_nodes_by_node_hosts(ios); break;
        case start_order::node_host_reverse: init_nodes_by_node_hosts_reverse(ios); break;
        case start_order::host_node: init_nodes_by_hosts_node(ios); break;
        default:
            MT_BOOST_FAIL("Unknown start_order was provided");
        }
    } catch (const std::exception& e) {
        MT_BOOST_FAIL("Failed to init nodes: " << e.what());
    } catch (...) {
        MT_BOOST_FAIL("Failed to init nodes for unknown reason");
    }
}

void nodes_tester_t::init_nodes_by_node_hosts(boost::asio::io_context& ios) {
    for (const auto& p : params_) {
        for (int host_id = 0; host_id < p.hosts; ++host_id) {
            auto new_node = dmn::make_node(ios, graph_, p.node_name, host_id);
            MT_BOOST_TEST(!!new_node);

            nodes_.push_back(std::move(new_node));
            switch (p.act) {
            case actions::generate:
                nodes_.back()->callback_ = [this](auto& s) { generate_sequence(&s); };
                break;
            case actions::remember:
                nodes_.back()->callback_ = [this](auto& s) { remember_sequence(&s); };
                break;
            case actions::resend:
                nodes_.back()->callback_ = [this](auto& s) { resend_sequence(&s); };
                break;
            }
        }
    }
}
void nodes_tester_t::init_nodes_by_node_hosts_reverse(boost::asio::io_context& ios) {
    for (unsigned i = 0; i < params_.size(); ++i) {
        const auto& p = *(params_.begin() + params_.size() - i - 1);

        for (int host_id = 0; host_id < p.hosts; ++host_id) {
            auto new_node = dmn::make_node(ios, graph_, p.node_name, host_id);
            MT_BOOST_TEST(!!new_node);


            nodes_.push_back(std::move(new_node));
            switch (p.act) {
            case actions::generate:
                nodes_.back()->callback_ = [this](auto& s) { generate_sequence(&s); };
                break;
            case actions::remember:
                nodes_.back()->callback_ = [this](auto& s) { remember_sequence(&s); };
                break;
            case actions::resend:
                nodes_.back()->callback_ = [this](auto& s) { resend_sequence(&s); };
                break;
            }
        }
    }
}
void nodes_tester_t::init_nodes_by_hosts_node(boost::asio::io_context& ios) {
    bool was_host_initialized = true;
    for (int host_id = 0; was_host_initialized; ++host_id) {
        was_host_initialized = false;

        for (const auto& p : params_) {
            if (host_id >= p.hosts) {
                continue;
            }
            was_host_initialized = true;

            auto new_node = dmn::make_node(ios, graph_, p.node_name, host_id);
            MT_BOOST_TEST(!!new_node);

            nodes_.push_back(std::move(new_node));
            switch (p.act) {
            case actions::generate:
                nodes_.back()->callback_ = [this](auto& s) { generate_sequence(&s); };
                break;
            case actions::remember:
                nodes_.back()->callback_ = [this](auto& s) { remember_sequence(&s); };
                break;
            case actions::resend:
                nodes_.back()->callback_ = [this](auto& s) { resend_sequence(&s); };
                break;
            }
        }
    }

}

void nodes_tester_t::test(start_order order) {
    nodes_.reserve(params_.size());
    {
        boost::asio::io_context ios{threads_count_};
        nodes_guard ng{ios, nodes_};
        init_nodes_by(order, ios);

        ethalon_sequences_ = seq_ethalon();
        run_impl(ios);
    }

    nodes_.clear();
    test_function_called_ = true;
    MT_BOOST_TEST(sequences_ == ethalon_sequences_);
}


void nodes_tester_t::test_cancellation(start_order order) {
    nodes_.reserve(params_.size());
    {
        boost::asio::io_context ios{threads_count_};
        nodes_guard ng{ios, nodes_};
        init_nodes_by(order, ios);

        sequence_max(std::numeric_limits<decltype(max_seq_)>::max());
        ios.post([&ios]() {
            ios.stop();
        });
        run_impl(ios);
    }

    nodes_.clear();
    test_function_called_ = true;
}

void nodes_tester_t::test_immediate_cancellation(start_order order) {
    nodes_.reserve(params_.size());

    {
        boost::asio::io_context ios{threads_count_};
        nodes_guard ng{ios, nodes_};
        init_nodes_by(order, ios);

        ios.stop();
        run_impl(ios);
    }

    nodes_.clear();
    test_function_called_ = true;
}

namespace {
    bool is_port_in_use(unsigned short port) noexcept {
        static boost::asio::io_context ios;
        boost::asio::ip::tcp::acceptor a{ios};

        boost::system::error_code ec;
        a.open(boost::asio::ip::tcp::v4(), ec);
        if (ec) {
            return true;
        }
        a.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
        a.bind({boost::asio::ip::tcp::v4(), port}, ec);

        return ec == boost::system::errc::address_in_use;
    }
}
    
nodes_tester_t::nodes_tester_t(const links_t& links, std::initializer_list<node_params> params)
    : params_(params)
{
    try {
        unsigned short port_num = 41000;
        graph_ = "digraph test {\n";
        for (const auto& p : params_) {
            graph_ += p.node_name;
            graph_ += " [hosts = \"";
            for (int i = 0; i < p.hosts; ++i) {
                if (i) {
                    graph_ += ';';
                }
                while (is_port_in_use(port_num)) ++ port_num;
                graph_ += "127.0.0.1:" + std::to_string(port_num);
                ++ port_num;
            }
            graph_ += "\"]\n";
        }
        graph_ += links.data + "}";
    } catch (const std::exception& e) {
        MT_BOOST_FAIL("Failed to generate graph: " << e.what());
    }  catch (...) {
        MT_BOOST_FAIL("Unknown exception during graph generation");
    }
}

nodes_tester_t::nodes_tester_t(const graph_t& graph, std::initializer_list<node_params> params)
    : graph_(graph.data)
{}

nodes_tester_t::~nodes_tester_t() noexcept {
    MT_BOOST_TEST(test_function_called_);
}
    
}
