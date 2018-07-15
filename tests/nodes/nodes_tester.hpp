#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/asio/io_context.hpp>

namespace dmn {
    class node_base_t;
}

namespace tests {

struct links_t {
    std::string data;
};

struct graph_t {
    std::string data;
};

enum class actions {
    generate,
    remember,
    resend,
};


enum class start_order: int {
    node_host,
    node_host_reverse,
    host_node,

    end_,
};

enum class percent: int {};

template <class T>
constexpr auto operator*(percent lhs, T value) {
    return static_cast<double>(lhs) / 100 * value;
}

template <class T>
constexpr auto operator*(T value, percent rhs) {
    return rhs * value;
}

constexpr percent operator "" _perc(unsigned long long val) noexcept {
    return static_cast<percent>(val);
}

struct node_params {
    const char* node_name;
    actions act;
    int hosts = 1;
};

struct node_name_and_host_id {
    const char* node_name;
    int host_id;
};

inline bool operator==(node_name_and_host_id lhs, node_name_and_host_id rhs) noexcept {
    return !std::strcmp(lhs.node_name, rhs.node_name) && lhs.host_id == rhs.host_id;
}

class nodes_tester_t {
    nodes_tester_t(const nodes_tester_t&) = delete;
    nodes_tester_t& operator=(const nodes_tester_t&) = delete;

    std::string graph_;
    std::initializer_list<node_params> params_;
    std::initializer_list<node_name_and_host_id> skip_list_;
    mutable bool test_function_called_ = false;
    percent match_ = 100_perc;

    int threads_count_ = 1;
    std::vector<std::unique_ptr<dmn::node_base_t>> nodes_;

    void run_impl(boost::asio::io_context& ios);

    int max_seq_ = 10;

    mutable std::atomic<int> sequence_counter_{0};
    mutable std::mutex  seq_mutex_;
    mutable std::vector<unsigned> sequences_;
    mutable int accepted_packets_ = 0;
    mutable bool shutdown_ = false;
    std::vector<unsigned> ethalon_sequences_;

    void set_seq_and_ethalon();
    void store_new_node(std::unique_ptr<dmn::node_base_t>&& node, actions action);

    void generate_sequence(void* s_void) const;
    void remember_sequence(void* s_void) const;
    void resend_sequence(void* s_void) const;

    void init_nodes_by(start_order order, boost::asio::io_context& ios);
    void init_nodes_by_node_hosts(boost::asio::io_context& ios);
    void init_nodes_by_node_hosts_reverse(boost::asio::io_context& ios);
    void init_nodes_by_hosts_node(boost::asio::io_context& ios);

    void validate_results() const;
public:
    nodes_tester_t(const links_t& links, std::initializer_list<node_params> params);
    nodes_tester_t(const graph_t& graph, std::initializer_list<node_params> params);

    nodes_tester_t& threads(int threads_count) {
        threads_count_ = threads_count;
        return *this;
    }

    nodes_tester_t& sequence_max(int seq) {
        max_seq_ = seq;
        return *this;
    }

    nodes_tester_t& skip(std::initializer_list<node_name_and_host_id> skip_list) {
        skip_list_ = skip_list;
        return *this;
    }

    void test(start_order order = start_order::node_host);
    void test_cancellation(start_order order = start_order::node_host);
    void test_immediate_cancellation(start_order order = start_order::node_host);
    void test_death(percent match);

    ~nodes_tester_t() noexcept;
};

template <unsigned Offset, unsigned BitsPerHost = 1>
int hosts_count_from_num(unsigned num) {
    return ((num >> Offset) & ((1 << BitsPerHost) - 1)) + 1;
}

}

using tests::actions;
using tests::start_order;
using tests::nodes_tester_t;
using tests::hosts_count_from_num;
using tests::percent;
using tests::operator "" _perc;
