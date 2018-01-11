#pragma once

#include <iterator>
#include <boost/assert.hpp>


namespace dmn {

template <class Container>
struct circular_iterator {
    using value_type = typename Container::value_type;
    using iterator_category = std::forward_iterator_tag;

private:
    Container* const      c_ = nullptr;
    std::size_t           index_ = 0;
    std::size_t           iterations_remain_ = 0;

public:
    circular_iterator() noexcept = default;

    circular_iterator(Container& c, std::size_t start_index, std::size_t iterations_count) noexcept
        : c_(std::addressof(c))
        , index_(start_index < c.size() ? start_index : (c.size() ? start_index % c.size() : 0))
        , iterations_remain_(iterations_count)
    {}

    circular_iterator operator++() noexcept {  // prefix
        BOOST_ASSERT_MSG(iterations_remain_, "Iterated over the end");
        BOOST_ASSERT_MSG(c_, "Incrementing the end iterator");
        --iterations_remain_;
        ++ index_;
        if (index_ == c_->size()) {
            index_ = 0;
        }

        return *this;
    }

    circular_iterator operator++(int) noexcept { // postfix
        const auto it = *this;
        ++(*this);
        return it;
    }

    typename Container::reference operator*() const noexcept {
        return (*c_)[index_];
    }

    bool operator==(circular_iterator rhs) const noexcept {
        BOOST_ASSERT_MSG(c_ == nullptr || rhs.c_ == nullptr, "Comparing with a non end iterator");
        return iterations_remain_ == rhs.iterations_remain_;
    }

    bool operator!=(circular_iterator rhs) const noexcept {
        BOOST_ASSERT_MSG(c_ == nullptr || rhs.c_ == nullptr, "Comparing with a non end iterator");
        return iterations_remain_ != rhs.iterations_remain_;
    }
};

} // namespace dmn
