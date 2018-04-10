#include "nodes_tester.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>

#include "node_base.hpp"
#include "stream.hpp"

namespace tests {

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
        s.stop();
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
        bool shutdown = false;
        {
            std::lock_guard<std::mutex> l(seq_mutex_);
            if (!res) {
                BOOST_CHECK(res);   // Not thread safe! Called under a mutex!
            }
            ++sequences_[seq];
            shutdown = (sequences_ == ethalon_sequences_);
        }
        if (shutdown) {
            for (const auto& node : nodes_) {
                node->shutdown_gracefully();
            }
        }
    }
}

void nodes_tester_t::resend_sequence(void* s_void) const {
    dmn::stream_t& s = *static_cast<dmn::stream_t*>(s_void);
    const auto data = s.get_data("seq");
    s.add(data.first, data.second, "seq");
}


void nodes_tester_t::run_impl() {
    constexpr auto ios_run = []() {
        dmn::node_base_t::ios().reset();
        dmn::node_base_t::ios().run();
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


void nodes_tester_t::init_nodes_by(start_order order) {
    switch (order) {
    case start_order::node_host: init_nodes_by_node_hosts(); break;
    case start_order::node_host_reverse: init_nodes_by_node_hosts_reverse(); break;
    case start_order::host_node: init_nodes_by_hosts_node(); break;
    default:
        BOOST_TEST(false);
    }
}

void nodes_tester_t::init_nodes_by_node_hosts() {
    for (const auto& p : params_) {
        for (int host_id = 0; host_id < p.hosts; ++host_id) {
            auto new_node = dmn::make_node(graph_, p.node_name, host_id);
            BOOST_TEST(!!new_node);

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
void nodes_tester_t::init_nodes_by_node_hosts_reverse() {
    for (unsigned i = 0; i < params_.size(); ++i) {
        const auto& p = *(params_.begin() + params_.size() - i - 1);

        for (int host_id = 0; host_id < p.hosts; ++host_id) {
            auto new_node = dmn::make_node(graph_, p.node_name, host_id);
            BOOST_TEST(!!new_node);

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
void nodes_tester_t::init_nodes_by_hosts_node() {
    bool was_host_initialized = true;
    for (int host_id = 0; was_host_initialized; ++host_id) {
        was_host_initialized = false;

        for (const auto& p : params_) {
            if (host_id >= p.hosts) {
                continue;
            }
            was_host_initialized = true;

            auto new_node = dmn::make_node(graph_, p.node_name, host_id);
            BOOST_TEST(!!new_node);

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

    init_nodes_by(order);

    ethalon_sequences_ = seq_ethalon();
    run_impl();
    nodes_.clear();

    BOOST_TEST(sequences_ == ethalon_sequences_);
    test_function_called_ = true;
}


void nodes_tester_t::test_cancellation(start_order order) {
    nodes_.reserve(params_.size());

    init_nodes_by(order);

    sequence_max(std::numeric_limits<decltype(max_seq_)>::max());
    dmn::node_base_t::ios().post(
        [this]() {
            for (const auto& node : nodes_) {
                node->shutdown_gracefully();
            }
        }
    );
    run_impl();
    test_function_called_ = true;
}

void nodes_tester_t::test_immediate_cancellation(start_order order) {
    nodes_.reserve(params_.size());

    init_nodes_by(order);
    for (const auto& node : nodes_) {
        node->shutdown_gracefully();
    }

    run_impl();
    test_function_called_ = true;
}
    
nodes_tester_t::nodes_tester_t(const links_t& links, std::initializer_list<node_params> params)
    : params_(params)
{
    unsigned short port_num = 44000;
    graph_ = "digraph test {\n";
    for (const auto& p : params_) {
        graph_ += p.node_name;
        graph_ += " [hosts = \"";
        for (int i = 0; i < p.hosts; ++i) {
            if (i) {
                graph_ += ';';
            }
            graph_ += "127.0.0.1:" + std::to_string(port_num);
            ++ port_num;
        }
        graph_ += "\"]\n";
    }
    graph_ += links.data + "}";
}

nodes_tester_t::nodes_tester_t(const graph_t& graph, std::initializer_list<node_params> params)
    : graph_(graph.data)
{}

nodes_tester_t::~nodes_tester_t() noexcept {
    BOOST_TEST(test_function_called_);
}
    
}
