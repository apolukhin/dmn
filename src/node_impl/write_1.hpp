#pragma once

#include "node_base.hpp"
#include "netlink.hpp"
#include "silent_mt_queue.hpp"
#include "work_counter.hpp"

#include <mutex>
#include <vector>


#include "packet_network.hpp"

namespace dmn {

struct netlink_with_packet {
    netlink_write_ptr link;
    packet_network_t packet;
};

inline bool operator<(const netlink_with_packet& lhs, const netlink_write_t* rhs) noexcept {
    return lhs.link.get() < rhs;
}

inline bool operator<(const netlink_with_packet& lhs, const netlink_with_packet& rhs) noexcept {
    return lhs.link.get() < rhs.link.get();
}

inline bool operator<(const netlink_write_t* lhs, const netlink_with_packet& rhs) noexcept {
    return lhs < rhs.link.get();
}

struct edges_out_t {
    std::mutex                                                  netlinks_mutex_;
    std::vector<netlink_with_packet>                            netlinks_;
    silent_mt_queue<packet_network_t>                           data_to_send_;

    auto find_netlink(netlink_write_t* link, std::unique_lock<std::mutex>& /*guard*/) {
        const auto pair = std::equal_range(netlinks_.begin(), netlinks_.end(), link);
        BOOST_ASSERT(pair.second - pair.first == 1);
        return pair.first;
    }

    void try_send() {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        for (auto& v: netlinks_) {   // TODO: balancing
            netlink_write_t::guard_t lock = v.link->try_lock();
            if (!lock) {
                continue;
            }

            auto p = data_to_send_.try_pop();
            if (!p) {
                return;
            }
            v.packet = std::move(*p);

            auto* connection = v.link.get();
            auto buf = v.packet.const_buffer();
            guard.unlock();

            connection->async_send(std::move(lock), buf);
            break;
        }
    }

    void try_steal_work(netlink_write_t* link, netlink_write_t::guard_t&& guard) {
        // Work stealing
        auto v = data_to_send_.try_pop();
        if (!v) {
            return;
        }

        std::unique_lock<std::mutex> g(netlinks_mutex_);
        auto it = find_netlink(link, g);
        it->packet = std::move(*v);
        auto buf = it->packet.const_buffer();
        g.unlock();

        link->async_send(std::move(guard), buf);
    }
};

class node_impl_write_1: public virtual node_base_t, private edges_out_t {
    work_counter_t                  pending_writes_;    

    void on_error(netlink_write_t* link, const boost::system::error_code& e, netlink_write_t::guard_t&& guard) {
        if (state() == node_state::RUN) {
            std::unique_lock<std::mutex> g(netlinks_mutex_);
            auto it = find_netlink(link, g);
            auto p = std::move(it->packet);
            g.unlock();

            if (!p.empty()) {
                data_to_send_.silent_push_front(std::move(p));
            }

            link->async_connect(std::move(guard)); // TODO: dealy?
            try_send();
            return;
        }

        std::unique_lock<std::mutex> g(netlinks_mutex_);
        auto it = find_netlink(link, g);
        auto l = std::move(*it);
        netlinks_.erase(it);
        g.unlock();

        if (!l.packet.empty()) {
            data_to_send_.silent_push_front(std::move(l.packet));
        }

        guard.unlock();
        (void)l; // TODO: log issue
    }

    void on_operation_finished(netlink_write_t* link, netlink_write_t::guard_t&& guard) {
        pending_writes_.remove();

        try_steal_work(link, std::move(guard));
    }

public:
    node_impl_write_1() {
        const auto edges_out = boost::out_edges(
            this_node_descriptor,
            config
        );

        BOOST_ASSERT(edges_out.second - edges_out.first == 1);
        const vertex_t& out_vertex = config[target(*edges_out.first, config)];

        pending_writes_.add(out_vertex.hosts.size());
        for (const auto& host : out_vertex.hosts) {

            auto link = netlink_write_t::construct(
                host.first.c_str(),
                host.second,
                ios(),
                [this](auto* link, const auto& e, auto&& guard) { on_error(link, e, std::move(guard)); },
                [this](auto* link, auto&& guard) { on_operation_finished(link, std::move(guard)); }
            );
            auto* link_ptr = link.get();
            netlinks_.emplace_back(netlink_with_packet{std::move(link), packet_network_t{}});
        }

        std::sort(netlinks_.begin(), netlinks_.end());
    }

    void on_stop_writing() noexcept override final {
        if (!pending_writes_.get()) {
            no_more_writers();
        }
    }

    void on_stoped_writing() noexcept override final {
        BOOST_ASSERT(!pending_writes_.get());
        BOOST_ASSERT(!data_to_send_.try_pop());

        std::lock_guard<std::mutex> g(netlinks_mutex_);
        netlinks_.clear();
    }

    void on_packet_accept(packet_t packet) override final {
        pending_writes_.add();

        packet_t data = call_callback(std::move(packet));

        data_to_send_.silent_push(packet_network_t{std::move(data)});

        // Rechecking for case when write operation was finished before we pushed into the data_to_send_
        try_send();
    }

    ~node_impl_write_1() noexcept = default;
};

}
