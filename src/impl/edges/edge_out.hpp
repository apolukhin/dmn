#pragma once

#include "impl/circular_iterator.hpp"
#include "impl/lazy_array.hpp"
#include "impl/silent_mt_queue.hpp"
#include "impl/net/netlink.hpp"
#include "impl/net/packet_network.hpp"
#include "impl/net/tcp_write_proto.hpp"
#include "impl/packet.hpp"

namespace dmn {

template <class Packet>
struct edge_out_t {
    using link_t = netlink_t<Packet, tcp_write_proto_t>;
    using netlinks_t = lazy_array<link_t>;

private:
    const std::uint16_t     edge_id_for_receiver_;

protected:
    netlinks_t              netlinks_{}; // `const` after set_links()

    template <class Link>
    static boost::asio::const_buffers_1 get_buf(netlink_t<packet_network_t, Link>& link, packet_network_t v) noexcept {
        BOOST_ASSERT(!empty_packet(v));

        auto p = std::move(v); // workaround for self move
        link.packet = std::move(p);

        return link.packet.const_buffer();
    }

    template <class Link>
    static boost::asio::const_buffers_1 get_buf(netlink_t<boost::asio::const_buffer, Link>& link, boost::asio::const_buffer v) noexcept {
        BOOST_ASSERT(!empty_packet(v));

        link.packet = std::move(v);
        return boost::asio::const_buffers_1{link.packet};
    }

    template <class Link>
    static std::array<boost::asio::const_buffer, 2> get_buf(netlink_t<std::pair<packet_header_t, boost::asio::const_buffer>, Link>& link, const std::pair<packet_header_t, boost::asio::const_buffer>& v) noexcept {
        BOOST_ASSERT(!empty_packet(v));

        link.packet = std::move(v);
        return {
            boost::asio::const_buffer{static_cast<const void*>(&link.packet.first), sizeof(packet_header_t)},
            link.packet.second
        };
    }

    static bool empty_packet(boost::asio::const_buffer buf) noexcept {
        return boost::asio::buffer_size(buf) == 0;
    }

    static bool empty_packet(const packet_network_t& p) noexcept {
        return p.empty();
    }

    static bool empty_packet(const std::pair<packet_header_t, boost::asio::const_buffer>& /*p*/) noexcept {
        return false;
    }

public:
    explicit edge_out_t(std::uint16_t edge_id_for_receiver)
        : edge_id_for_receiver_(edge_id_for_receiver)
    {}

    std::uint16_t edge_id_for_receiver() const noexcept {
        return edge_id_for_receiver_;
    }

    static link_t& link_from_guard(const tcp_write_proto_t::guard_t& guard) noexcept {
        BOOST_ASSERT_MSG(guard, "Empty link guard");
        return static_cast<link_t&>(*guard.mutex());
    }

    void preinit_links(std::size_t count) {
        netlinks_.init(count);
    }

    template <class... Args>
    void inplace_construct_link(std::size_t i, Args&&... args) {
        netlinks_.inplace_construct(i, std::forward<Args>(args)...);
    }

    void connect_links() noexcept {
        for (auto& v : netlinks_) {
            v.async_reconnect(v.try_lock());
        }
    }

    void close_links() noexcept {
        for (auto& v: netlinks_) {
            tcp_write_proto_t::guard_t lock = v.try_lock();
            if (lock) {
                v.close(std::move(lock));
            } else {
                // Link is already closed // TODO: validate
            }
        }
    }

    virtual void try_steal_work(tcp_write_proto_t::guard_t guard) = 0;
    virtual void reschedule_packet_from_link(const tcp_write_proto_t::guard_t& guard) = 0;
    virtual void push(wave_id_t wave, Packet p) = 0;
    virtual ~edge_out_t() = default;
};



template <class Packet>
struct edge_out_round_robin_t final: public edge_out_t<Packet> {
    using base_t = edge_out_t<Packet>;

    using queue_t = silent_mt_queue<Packet>;
    using link_t = typename base_t::link_t;
    using netlinks_t = typename base_t::netlinks_t;

private:
    std::atomic_uintmax_t   last_used_link{0};
    queue_t                 data_to_send_{};

    template <class Container>
    struct round_robin_balancer_t {
        Container&             c_;
        const std::size_t      start_index_;

        circular_iterator<Container> begin() const noexcept {
            return circular_iterator<Container>(c_, start_index_, c_.size());
        }

        circular_iterator<Container> end() const noexcept {
            return circular_iterator<Container>();
        }
    };

    round_robin_balancer_t<netlinks_t> links_balancer() noexcept {
        return {base_t::netlinks_, last_used_link.fetch_add(1, std::memory_order_relaxed)};
    }

    void try_send() {
        for (auto& v: links_balancer()) {
            tcp_write_proto_t::guard_t lock = v.try_lock();
            if (!lock) {
                continue;
            }

            auto p = data_to_send_.try_pop();
            if (!p) {
                return;
            }

            const auto buf = base_t::get_buf(v, std::move(*p));
            v.async_send(std::move(lock), buf);
            break;
        }
    }

public:
    explicit edge_out_round_robin_t(std::uint16_t edge_id_for_receiver)
        : base_t(edge_id_for_receiver)
    {}

    void assert_no_more_data() noexcept {
        BOOST_ASSERT_MSG(!data_to_send_.try_pop(), "Have data to send");
    }

    void try_steal_work(tcp_write_proto_t::guard_t guard) final {
        auto v = data_to_send_.try_pop();
        if (!v) {
            return;
        }

        auto& link = base_t::link_from_guard(guard);
        const auto buf = base_t::get_buf(link, std::move(*v));
        link.async_send(std::move(guard), buf);
    }

    void reschedule_packet_from_link(const tcp_write_proto_t::guard_t& guard) final {
        auto& link = base_t::link_from_guard(guard);
        auto p = std::move(link.packet);

        BOOST_ASSERT_MSG(!base_t::empty_packet(p), "Scheduling an empty packet without headers for push_immediate sending. This must not be produced by accepting vertexes");
        data_to_send_.silent_push_front(std::move(p));
        try_send();
    }

    void push(wave_id_t /*wave*/, Packet p) final {
        BOOST_ASSERT_MSG(!base_t::empty_packet(p), "Scheduling an empy packet without headers for sending. This must not be produced by accepting vertexes");
        data_to_send_.silent_push(std::move(p));
        try_send(); // Rechecking for case when some write operation was finished before we pushed into the data_to_send_
    }
};

template <class Packet>
struct edge_out_exact_t final: public edge_out_t<Packet> {
    using base_t = edge_out_t<Packet>;

    using queue_t = silent_mt_queue<Packet>;
    using link_t = typename base_t::link_t;
    using netlinks_t = typename base_t::netlinks_t;

private:
    std::atomic_uintmax_t   last_used_link{0};
    queue_t                 data_to_send_{};

    template <class Container>
    struct round_robin_balancer_t {
        Container&             c_;
        const std::size_t      start_index_;

        circular_iterator<Container> begin() const noexcept {
            return circular_iterator<Container>(c_, start_index_, c_.size());
        }

        circular_iterator<Container> end() const noexcept {
            return circular_iterator<Container>();
        }
    };

    round_robin_balancer_t<netlinks_t> links_balancer() noexcept {
        return {base_t::netlinks_, last_used_link.fetch_add(1, std::memory_order_relaxed)};
    }

    void try_send() {
        for (auto& v: links_balancer()) {
            tcp_write_proto_t::guard_t lock = v.try_lock();
            if (!lock) {
                continue;
            }

            auto p = data_to_send_.try_pop();
            if (!p) {
                return;
            }

            const auto buf = base_t::get_buf(v, std::move(*p));
            v.async_send(std::move(lock), buf);
            break;
        }
    }

public:
    explicit edge_out_exact_t(std::uint16_t edge_id_for_receiver)
        : base_t(edge_id_for_receiver)
    {}

    void assert_no_more_data() noexcept {
        BOOST_ASSERT_MSG(!data_to_send_.try_pop(), "Have data to send");
    }

    void try_steal_work(tcp_write_proto_t::guard_t guard) final {
        auto v = data_to_send_.try_pop();
        if (!v) {
            return;
        }

        auto& link = base_t::link_from_guard(guard);
        const auto buf = base_t::get_buf(link, std::move(*v));
        link.async_send(std::move(guard), buf);
    }

    void reschedule_packet_from_link(const tcp_write_proto_t::guard_t& guard) final {
        auto& link = base_t::link_from_guard(guard);
        auto p = std::move(link.packet);

        BOOST_ASSERT_MSG(!base_t::empty_packet(p), "Scheduling an empty packet without headers for push_immediate sending. This must not be produced by accepting vertexes");
        data_to_send_.silent_push_front(std::move(p));
        try_send();
    }

    void push(wave_id_t /*wave*/, Packet p) final {
        BOOST_ASSERT_MSG(!base_t::empty_packet(p), "Scheduling an empy packet without headers for sending. This must not be produced by accepting vertexes");
        data_to_send_.silent_push(std::move(p));
        try_send(); // Rechecking for case when some write operation was finished before we pushed into the data_to_send_
    }
};


}
