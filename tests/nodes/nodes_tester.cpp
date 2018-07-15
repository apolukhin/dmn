#include "nodes_tester.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_unique.hpp>
#include <thread>

#include "node_base.hpp"
#include "stream.hpp"

namespace tests {

namespace {
    // Depends on OS context switch time and time slices that are given to the process.
constexpr auto max_time_to_wait_for_a_single_task = std::chrono::milliseconds{5000};
constexpr auto max_time_to_wait_after_initing_shutdown = std::chrono::milliseconds{4};

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

    struct shutdown_generator{};
}

#define MT_BOOST_TEST(x) if (!(x)) { std::lock_guard<std::mutex> lock{g_tests_mutex}; BOOST_TEST(x); }
#define MT_BOOST_FAIL(x) { std::lock_guard<std::mutex> lock{g_tests_mutex}; BOOST_FAIL(x); }
#define MT_BOOST_WARN_IF_NOT(x, msg) if (!(x)) { std::lock_guard<std::mutex> lock{g_tests_mutex}; BOOST_WARN_MESSAGE(x, msg); }

void nodes_tester_t::set_seq_and_ethalon() {
    ethalon_sequences_.clear();
    ethalon_sequences_.resize(max_seq_, 1);

    sequences_.clear();
    sequences_.resize(max_seq_, 0);
}

void nodes_tester_t::generate_sequence(void* s_void) const {
    dmn::stream_t& s = *static_cast<dmn::stream_t*>(s_void);
    const int seq = sequence_counter_.fetch_add(1);

    if (seq < max_seq_) {
        auto data = boost::lexical_cast<std::string>(seq);
        s.add(data.data(), data.size(), "seq");
    }

    if (seq >= max_seq_) {
        throw shutdown_generator{};
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


        auto do_shutdown = [this] () {
            for (const auto& node : nodes_) {
                node->ios().stop();
            }
        };

        bool shutdown_immediate = false;
        bool shutdown_delayed = false;
        {
            std::lock_guard<std::mutex> l(seq_mutex_);
            ++sequences_[seq];
            ++accepted_packets_;

            if (shutdown_) {
                // Not first shutdown request
                return;
            }

            shutdown_immediate = (accepted_packets_ == max_seq_);
            shutdown_delayed = (accepted_packets_ >= max_seq_ * match_);
            shutdown_ = (shutdown_immediate || shutdown_delayed);
        }

        if (shutdown_immediate) {
            do_shutdown();
        } else if (shutdown_delayed) {
            auto timer_ptr = boost::make_unique<boost::asio::steady_timer>(nodes_.back()->ios());
            auto& timer = *timer_ptr;
            timer.expires_after(max_time_to_wait_after_initing_shutdown);
            timer.async_wait([do_shutdown = do_shutdown, t = std::move(timer_ptr)] (auto /*ignore*/) {
                do_shutdown();
            });
        }
    }
}

void nodes_tester_t::resend_sequence(void* s_void) const {
    dmn::stream_t& s = *static_cast<dmn::stream_t*>(s_void);
    const auto data = s.get_data("seq");
    s.add(data.first, data.second, "seq");
}


void nodes_tester_t::run_impl(boost::asio::io_context& ios) {
    auto ios_run = [&ios, this]() {
        try {

            bool ok_to_restart;
            do {
                ok_to_restart = false;
                try {
                    ios.run_for(max_time_to_wait_for_a_single_task);
                } catch (shutdown_generator g) {
                    ok_to_restart = true;
                }
            } while (ok_to_restart);

        } catch (const std::exception& e) {
            MT_BOOST_FAIL("Sudden exception during run: " << e.what());
        } catch (...) {
            MT_BOOST_FAIL("Sudden unknown exception during run");
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(threads_count_);

    for (int i = 1; i < threads_count_; ++i) {
        threads.emplace_back(ios_run);
    }
    ios_run();
    for (auto& t: threads) {
        t.join();
    }
    threads.clear();
}


void nodes_tester_t::store_new_node(std::unique_ptr<dmn::node_base_t>&& node, actions action) {
    MT_BOOST_TEST(!!node);

    nodes_.push_back(std::move(node));
    switch (action) {
    case actions::generate:
        nodes_.back()->callback_ = [this](auto& s) { generate_sequence(&s); };
        break;
    case actions::remember:
        nodes_.back()->callback_ = [this](auto& s) { remember_sequence(&s); };
        break;
    case actions::resend:
        nodes_.back()->callback_ = [this](auto& s) { resend_sequence(&s); };
        break;
    default:
        MT_BOOST_FAIL("Unknown action was provided");
    }
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
            if (std::find(skip_list_.begin(), skip_list_.end(), node_name_and_host_id{p.node_name, host_id}) != skip_list_.end()) {
                continue;
            }
            store_new_node(dmn::make_node(ios, graph_, p.node_name, host_id), p.act);
        }
    }
}

void nodes_tester_t::init_nodes_by_node_hosts_reverse(boost::asio::io_context& ios) {
    for (unsigned i = 0; i < params_.size(); ++i) {
        const auto& p = *(params_.begin() + params_.size() - i - 1);

        for (int host_id = 0; host_id < p.hosts; ++host_id) {
            if (std::find(skip_list_.begin(), skip_list_.end(), node_name_and_host_id{p.node_name, host_id}) != skip_list_.end()) {
                continue;
            }
            store_new_node(dmn::make_node(ios, graph_, p.node_name, host_id), p.act);
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

            if (std::find(skip_list_.begin(), skip_list_.end(), node_name_and_host_id{p.node_name, host_id}) != skip_list_.end()) {
                continue;
            }
            store_new_node(dmn::make_node(ios, graph_, p.node_name, host_id), p.act);
        }
    }
}


void nodes_tester_t::validate_results() const {
    test_function_called_ = true;

    int diff = 0;
    for (unsigned i = 0; i < ethalon_sequences_.size(); ++i) {
        MT_BOOST_TEST(sequences_[i] < 2);
        if (ethalon_sequences_[i] != sequences_[i]) ++diff;
    }

    const int answers_ok_to_loose = max_seq_ - max_seq_ * match_;
    MT_BOOST_WARN_IF_NOT(answers_ok_to_loose >= diff, "Lost more answers than expected [" << answers_ok_to_loose << " vs " << diff << "]");


    if (match_ == 100_perc) {
        constexpr unsigned max_failures = 10;
        static unsigned failures = 0;
        if (sequences_ != ethalon_sequences_ && failures < max_failures) {
            // Requests did not fit in socket buffer and OS dropped them.
            ++failures;
            MT_BOOST_WARN_IF_NOT(sequences_ == ethalon_sequences_, "Not 100% match");

            // Giving the OS some time to recover.
            std::this_thread::sleep_for(std::chrono::milliseconds{250});
        } else {
            MT_BOOST_TEST(sequences_ == ethalon_sequences_);
        }
        if (sequences_ != ethalon_sequences_) {
            for (unsigned i = 0; i < ethalon_sequences_.size(); ++i) {
                MT_BOOST_WARN_IF_NOT(sequences_[i] == ethalon_sequences_[i], "Error at index " << i);
            }
        }
    } else {
        MT_BOOST_TEST(answers_ok_to_loose);

        const int percent = diff * 100 / answers_ok_to_loose;
        MT_BOOST_WARN_IF_NOT(
            percent <= 100,
            "Difference is " << diff << " (" << percent << "% of max allowed)."
        );
    }
}

void nodes_tester_t::test(start_order order) {
    set_seq_and_ethalon();
    {
        boost::asio::io_context ios{threads_count_};
        nodes_guard ng{ios, nodes_};
        init_nodes_by(order, ios);

        run_impl(ios);
    }

    nodes_.clear();
    validate_results();
}


void nodes_tester_t::test_cancellation(start_order order) {
    test_function_called_ = true;
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
}

void nodes_tester_t::test_immediate_cancellation(start_order order) {
    test_function_called_ = true;

    {
        boost::asio::io_context ios{threads_count_};
        nodes_guard ng{ios, nodes_};
        init_nodes_by(order, ios);

        ios.stop();
        run_impl(ios);
    }

    nodes_.clear();
}

void nodes_tester_t::test_death(percent match_in_param) {
    match_ = match_in_param;

    set_seq_and_ethalon();

    {
        // Nodes that must keep working after the death of other nodes
        boost::asio::io_context ios{threads_count_};
        nodes_guard ng{ios, nodes_};
        init_nodes_by(start_order::node_host, ios);

        const auto nodes_to_survive_count = nodes_.size();

        // Also initing nodes that are doomed to die
        std::vector<std::unique_ptr<dmn::node_base_t>> nodes_to_die;
        boost::asio::io_context ios_to_kill{threads_count_};
        for (auto& v: skip_list_) {
            try {
                store_new_node(
                    dmn::make_node(ios_to_kill, graph_, v.node_name, v.host_id),
                    std::find_if(params_.begin(), params_.end(), [v](const auto& val){ return !std::strcmp(val.node_name, v.node_name); })->act
                );
            } catch (const std::exception& e) {
                MT_BOOST_FAIL("Exception during doomed nodes construction:" << e.what());
                ios_to_kill.stop();
                throw;
            }
        }
        nodes_to_die.assign(
            std::make_move_iterator(nodes_.begin() + nodes_to_survive_count),
            std::make_move_iterator(nodes_.end())
        );
        nodes_.resize(nodes_to_survive_count);

        std::thread t_kill{[this, &ios, &ios_to_kill, &nodes_to_die] {
            try {
                nodes_guard ng{ios_to_kill, nodes_to_die};
                for (unsigned i = 0; i < skip_list_.size() * 2; ++i) {
                    try {
                        ios_to_kill.run_one_for(max_time_to_wait_for_a_single_task);
                    } catch (shutdown_generator g) {
                        // OK
                        (void)g;
                    }
                }
                ios_to_kill.stop();
            } catch (const std::exception& e) {
                MT_BOOST_FAIL("Sudden exception during run: " << e.what());
            } catch (...) {
                MT_BOOST_FAIL("Sudden unknown exception during run");
            }
        }};

        run_impl(ios);

        t_kill.join();
    }
    nodes_.clear();
    validate_results();
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

        boost::system::error_code ignore;
        a.close(ignore);

        return ec == boost::system::errc::address_in_use;
    }
}
    
nodes_tester_t::nodes_tester_t(const links_t& links, std::initializer_list<node_params> params)
    : params_(params)
{
    nodes_.reserve(params_.size());
    try {
        static unsigned short port_num = 21000;
        graph_.reserve(50 + 32 * params_.size());
        graph_ = "digraph test {\n";
        for (const auto& p : params_) {
            graph_ += p.node_name;
            graph_ += " [hosts = \"";
            for (int i = 0; i < p.hosts; ++i) {
                if (i) {
                    graph_ += ';';
                }
                while (is_port_in_use(port_num)) ++ port_num;
                graph_ += "127.0.0.1:";
                graph_ += std::to_string(port_num);
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
{
    nodes_.reserve(params_.size());
}

nodes_tester_t::~nodes_tester_t() noexcept {
    BOOST_TEST(test_function_called_);
}
    
}
