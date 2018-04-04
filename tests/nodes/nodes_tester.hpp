#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include "node_base.hpp"
#include "stream.hpp"

namespace tests {

enum class actions {
    generate,
    remember,
    resend,
};

class nodes_tester_t {
    nodes_tester_t(const nodes_tester_t&) = delete;
    nodes_tester_t& operator=(const nodes_tester_t&) = delete;

    std::vector<std::unique_ptr<dmn::node_base_t>> nodes_;
    int threads_count_ = 1;
    std::vector<std::thread> threads_;

    void poll_impl();

    int max_seq_ = 10;

    mutable std::atomic<int> sequence_counter_{0};
    mutable std::mutex  seq_mutex_;
    mutable std::map<int, unsigned> sequences_;

    std::map<int, unsigned> seq_ethalon() const;

    void generate_sequence(dmn::stream_t& s) const;
    void remember_sequence(dmn::stream_t& s) const;
    void resend_sequence(dmn::stream_t& s) const;

public:
    struct node_params {
        const char* node_name;
        actions act;
        int hosts = 1;
    };

    nodes_tester_t(const std::string& links, std::initializer_list<node_params> params);

    nodes_tester_t& threads(int threads_count) {
        threads_count_ = threads_count;
        return *this;
    }

    nodes_tester_t& sequence_max(int seq) {
        max_seq_ = seq;
        return *this;
    }

    void poll();

    ~nodes_tester_t() {
        BOOST_TEST(nodes_.empty());
    }
};

}

using tests::actioins;
using tests::nodes_tester_t;

