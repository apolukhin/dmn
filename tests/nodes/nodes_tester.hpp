#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace dmn {
    class node_base_t;
}

namespace tests {

enum class actions {
    generate,
    remember,
    resend,
};

struct node_params {
    const char* node_name;
    actions act;
    int hosts = 1;
};

class nodes_tester_t {
    nodes_tester_t(const nodes_tester_t&) = delete;
    nodes_tester_t& operator=(const nodes_tester_t&) = delete;

    const std::string links_;
    std::initializer_list<node_params> params_;
    bool test_function_called_ = false;

    int threads_count_ = 1;
    std::vector<std::thread> threads_;
    std::vector<std::unique_ptr<dmn::node_base_t>> nodes_;

    void run_impl();

    int max_seq_ = 10;

    mutable std::atomic<int> sequence_counter_{0};
    mutable std::mutex  seq_mutex_;
    mutable std::map<int, unsigned> sequences_;
    std::map<int, unsigned> ethalon_sequences_;

    std::map<int, unsigned> seq_ethalon() const;

    void generate_sequence(void* s_void) const;
    void remember_sequence(void* s_void) const;
    void resend_sequence(void* s_void) const;

public:
    nodes_tester_t(std::string links, std::initializer_list<node_params> params)
        : links_(std::move(links))
        , params_(params)
    {}

    nodes_tester_t& threads(int threads_count) {
        threads_count_ = threads_count;
        return *this;
    }

    nodes_tester_t& sequence_max(int seq) {
        max_seq_ = seq;
        return *this;
    }

    void test();

    ~nodes_tester_t() noexcept;
};

}

using tests::actions;
using tests::nodes_tester_t;

