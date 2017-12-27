#pragma once

#include "node_base.hpp"
#include "netlink.hpp"
#include "silent_mt_queue.hpp"
#include "work_counter.hpp"

#include <mutex>

namespace dmn {

class node_impl_write_1: public virtual node_base_t {
    work_counter_t                  pending_writes_;
    std::mutex                      netlinks_mutex_;
    std::deque<netlink_write_ptr>   netlinks_;

    silent_mt_queue<packet_t> data_to_send_;

    netlink_write_t::guard_t try_get_netlink() {
        std::unique_lock<std::mutex> guard(netlinks_mutex_);
        for (auto& link: netlinks_) {   // TODO: balancing
            netlink_write_t::guard_t lock = link->try_lock();
            if (lock) {
                return lock;
            }
        }

        return {};
    }

    void on_error(netlink_write_t* link, const boost::system::error_code& e, netlink_write_t::guard_t&& guard, dmn::packet_t&& p) {
        if (!p.empty()) {
            data_to_send_.silent_push_front(std::move(p));
        }

        if (state() == node_state::RUN) {
            link->async_connect(std::move(guard)); // TODO: dealy?
            return;
        }

        std::unique_lock<std::mutex> g(netlinks_mutex_);
        auto it = std::find_if(netlinks_.begin(), netlinks_.end(), [link](auto& v){
            return v.get() == link;
        });
        BOOST_ASSERT(it != netlinks_.end());

        auto l = std::move(*it);
        netlinks_.erase(it);
        g.unlock();

        guard.unlock();
        (void)l; // TODO: log issue
    }

    void on_operation_finished(netlink_write_t* link, netlink_write_t::guard_t&& guard) {
        pending_writes_.remove();

        // Work stealing
        auto v = data_to_send_.try_pop();
        if (v) {
            link->async_send(std::move(guard), std::move(*v));
        }
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
            netlinks_.emplace_back(netlink_write_t::construct(
                host.first.c_str(),
                host.second,
                ios(),
                [this](auto* link, const auto& e, auto&& guard, dmn::packet_t&& p) { on_error(link, e, std::move(guard), std::move(p)); },
                [this](auto* link, auto&& guard) { on_operation_finished(link, std::move(guard)); }
            ));
        }
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

        data_to_send_.silent_push(std::move(data));

        // Rechecking for case when write operation was finished before we pushed into the data_to_send_
        auto l = try_get_netlink();
        if (l) {
            auto v = data_to_send_.try_pop();
            if (!v) {
                return;
            }

            auto& link = *l.mutex();
            link.async_send(std::move(l), std::move(*v));
        }
    }

    ~node_impl_write_1() noexcept = default;
};

}
