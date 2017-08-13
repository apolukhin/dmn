#pragma once

#include <boost/container/flat_map.hpp>

namespace dmn {

class message_t {
    boost::container::flat_map<std::string, std::vector<unsigned char> > data_;

public:
    void add(const void* data, std::size_t size, const char* type) {
        data_[type].assign(
            static_cast<const unsigned char*>(data),
            static_cast<const unsigned char*>(data) + size
        );
    }
};

}
