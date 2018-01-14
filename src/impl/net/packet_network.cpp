#include "packet_network.hpp"
#include <boost/endian/conversion.hpp>

namespace dmn {


packet_network_t::packet_network_t(packet_t&& n) noexcept
    : packet_t(std::move(n))
{
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "");
#endif
}

packet_types_enum packet_network_t::packet_type() const noexcept {
    return header().packet_type;
}

std::uint32_t packet_network_t::expected_body_size() const noexcept {
    return header().size;
}

std::uint32_t packet_network_t::actual_body_size() const noexcept {
    return data_.size() > sizeof(header()) ? data_.size() - sizeof(header()) : 0;
}

std::uint16_t packet_network_t::edge_id_from_packet() const noexcept {
    return header().edge_id;
}

wave_id_t packet_network_t::wave_id_from_packet() const noexcept {
    return header().wave_id;
}

void packet_network_t::merge_packet(packet_network_t&& in) {    // TODO: This is suboptimal. Make zero-copy here
    BOOST_ASSERT_MSG(header().wave_id == in.header().wave_id, "Merging packets of different waves. Error in logic");
    BOOST_ASSERT_MSG(in.data_.cbegin() + sizeof(packet_header_t) + in.header().size == in.data_.cend(), "Packet is corrupted: data size and data received missmatch");

    const auto data_begin = in.data_.data() + sizeof(packet_header_t);
    data_.insert(data_.end(), data_begin, data_begin + in.header().size);
    header().size += in.header().size;
}


packet_t packet_network_t::to_native() && noexcept {
    return packet_t{std::move(data_)};
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "");
#endif
}

} // namespace dmn
