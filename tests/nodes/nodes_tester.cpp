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
        std::lock_guard<std::mutex> l(seq_mutex_);
        BOOST_CHECK(res);   // Not thread safe! Called under a mutex!
        ++sequences_[seq];
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

void nodes_tester_t::test() {
    std::vector<std::unique_ptr<dmn::node_base_t>> nodes;
    nodes.reserve(params_.size());

    unsigned short port_num = 44000;
    std::string g = "digraph test {\n";
    for (const auto& p : params_) {
        g += p.node_name;
        g += " [hosts = \"";
        for (int i = 0; i < p.hosts; ++i) {
            if (i) {
                g += ';';
            }
            g += "127.0.0.1:" + std::to_string(port_num);
            ++ port_num;
        }
        g += "\"]\n";
    }
    g += links_ + "}";

    for (const auto& p : params_) {
        for (int host_id = 0; host_id < p.hosts; ++host_id) {
            auto new_node = dmn::make_node(g, p.node_name, host_id);
            BOOST_TEST(!!new_node);

            nodes.push_back(std::move(new_node));
            switch (p.act) {
            case actions::generate:
                nodes.back()->callback_ = [this](auto& s) { generate_sequence(&s); };
                break;
            case actions::remember:
                nodes.back()->callback_ = [this](auto& s) { remember_sequence(&s); };
                break;
            case actions::resend:
                nodes.back()->callback_ = [this](auto& s) { resend_sequence(&s); };
                break;
            }

        }
    }


    dmn::node_base_t::ios().reset();
    // TODO: run generators in parallel
    dmn::node_base_t::ios().poll(); // Starting all the generators
    for (auto& node : nodes) {
        node->shutdown_gracefully();
    }
    run_impl();
    nodes.clear();


    BOOST_TEST(sequences_ == seq_ethalon());
    test_function_called_ = true;
}

}
