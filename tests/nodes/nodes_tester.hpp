#include <thread>
#include <vector>
#include <memory>
#include <boost/test/unit_test.hpp>


#include "node_base.hpp"

namespace tests {

class nodes_tester_t {
    nodes_tester_t(const nodes_tester_t&) = delete;
    nodes_tester_t& operator=(const nodes_tester_t&) = delete;

    std::vector<std::unique_ptr<dmn::node_base_t>> nodes_;
    int threads_count_ = 1;
    std::vector<std::thread> threads_;

    void poll_impl();

public:
    struct node_params {
        const char* node_name;
        void(*callback)(dmn::stream_t&);
        int hosts = 1;
    };

    nodes_tester_t(const std::string& links, std::initializer_list<node_params> params);

    nodes_tester_t& threads(int threads_count) {
        threads_count_ = threads_count;
    }

    void poll();

    ~nodes_tester_t() {
        BOOST_TEST(nodes_.empty());
    }
};

}
