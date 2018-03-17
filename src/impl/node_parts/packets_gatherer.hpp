#pragma once

#include "node_base.hpp"

#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/edges/edge_in.hpp"
#include "impl/net/tcp_read_proto.hpp"

#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <boost/container/small_vector.hpp>

#include <mutex>

namespace dmn {


class packets_gatherer_t {
    DMN_PINNED(packets_gatherer_t);

    std::mutex  packets_mutex_;
    using received_edge_ids_t = boost::container::small_vector<std::size_t, 16>;
    std::unordered_map<wave_id_t, std::pair<received_edge_ids_t, packet_network_t> > packets_gatherer_;

    const std::size_t edges_count_;

public:
    explicit packets_gatherer_t(std::size_t edge_count)
        : edges_count_(edge_count)
    {}

    boost::optional<packet_network_t> combine_packets(packet_network_t p) {
        boost::optional<packet_network_t> result;
        const auto wave_id = p.wave_id_from_packet();
        const auto edge_id = p.edge_id_from_packet();

        std::lock_guard<std::mutex> l(packets_mutex_);
        const auto it = packets_gatherer_.find(wave_id);
        if (it == packets_gatherer_.cend()) {
            auto packet_it = packets_gatherer_.emplace(
                wave_id,
                std::pair<received_edge_ids_t, packet_network_t>{{}, std::move(p)}
            ).first;
            packet_it->second.first.push_back(edge_id);
            return result;
        }

        auto& edge_counter = it->second.first;
        auto ins_it = std::lower_bound(edge_counter.begin(), edge_counter.end(), edge_id);
        if (ins_it != edge_counter.end() && *ins_it == edge_id) {
            // Duplicate packet.
            // TODO: Write to log
            return result;
        }

        edge_counter.insert(ins_it, edge_id);
        if (edge_counter.size() == edges_count_) {
            result.emplace(std::move(it->second.second));
            packets_gatherer_.erase(it);
            return result;
        }

        return result;
    }

    // TODO: drop partial packets after timeout
};

}
