#pragma once

#include <deque>
#include <mutex>
#include <boost/optional.hpp>
#include "utility.hpp"

namespace dmn {

template <class T>
class alignas(hardware_destructive_interference_size) silent_mt_queue {
    std::mutex      data_mutex_;
    std::deque<T>   data_;

public:
    inline void silent_push(T value);

    boost::optional<T> try_pop() {
        boost::optional<T> ret;

        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (!data_.empty()) {
                ret.emplace(std::move(data_.front()));
                data_.pop_front();
            }
        }

        return ret;

    }
};

template <class T>
void silent_mt_queue<T>::silent_push(T value) {
    // TODO: deal with that by using intrusive slist
    //static_assert(sizeof(silent_mt_queue<T>) <= hardware_constructive_interference_size, "");

    std::lock_guard<std::mutex> lock(data_mutex_);
    data_.push_back(std::move(value));
}

}
