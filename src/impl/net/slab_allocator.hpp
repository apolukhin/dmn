#pragma once

#include "utility.hpp"
#include <new>
#include <type_traits>
#include <algorithm>
#include <boost/config.hpp>
#include <boost/assert.hpp>

namespace dmn {

// Class to manage the memory to be used for custom allocation.
// It contains blocks of memory which may be returned for allocation
// requests. If the memory blocks are in use when an allocation request is made, the
// allocator delegates allocation to the global heap.
template <std::size_t SlabSize, std::size_t SlabsCount>
class slab_allocator_basic_t {
    DMN_PINNED(slab_allocator_basic_t);

public:
    enum slab_size_enum: std::size_t { slab_size = SlabSize };
    enum slabs_count_enum: std::size_t { slabs_count = SlabsCount};

    slab_allocator_basic_t() noexcept = default;

    void* allocate(std::size_t size) {
        if (BOOST_UNLIKELY(!size)) {
            return nullptr;
        }

        const std::size_t blocks_required = (size - 1) / sizeof(storage_t) + 1;
        return blocks_required == 1 ? allocate_single(size) : allocate_multiple(blocks_required, size);
    }

    void deallocate(void* pointer) noexcept {
        for (std::size_t i = 0; i < SlabsCount; ++i) {
            if (storages_ + i == pointer) {
                const in_use_t to_free = in_use_[i];
                std::fill(in_use_ + i, in_use_ + i + to_free, static_cast<in_use_t>(0));
                return;
            }
        }

        BOOST_ASSERT_MSG(!pointer, "Slab allocator failed to find slab in a list of known slabs");
        ::operator delete(pointer);
    }

#if DMN_DEBUG
    ~slab_allocator_basic_t() {
        for (auto v: in_use_) {
            BOOST_ASSERT_MSG(!v, "Slab allocator is destroyed before all the resources were freed");
        }
    }
#endif

private:
    void* allocate_multiple(const std::size_t blocks_required, std::size_t size) {
        // Complexity in worst case: O(storages_count_)
        for (std::size_t i = 0; i <= SlabsCount - blocks_required; ++i) {
            if (!!in_use_[i]) {
                continue;
            }

            // We have a single free block... Searching for free `blocks_required`
            std::size_t free_blocks = 1;
            for (; free_blocks < blocks_required; ++free_blocks) {
                if (!!in_use_[i + free_blocks]) {
                    break;
                }
            }

            if (free_blocks != blocks_required) {
                i += free_blocks;
                continue;
            }

            // Gotcha!
            in_use_[i] = static_cast<in_use_t>(blocks_required);
            std::fill(in_use_ + i + 1, in_use_ + i + blocks_required, (std::numeric_limits<in_use_t>::max)());
            return storages_ + i;
        }

        BOOST_ASSERT_MSG(false, "Slab allocator does not have enough empty slabs");
        return ::operator new(size);
    }

    void* allocate_single(std::size_t size) {
        for (std::size_t i = 0; i < SlabsCount; ++i) {
            if (!in_use_[i]) {
                in_use_[i] = static_cast<in_use_t>(1u);
                return storages_ + i;
            }
        }

        BOOST_ASSERT_MSG(false, "Slab allocator does not have enough empty slabs");
        return ::operator new(size);
    }

    // Whether the handler-based custom allocation storage has been used.
    using in_use_t = unsigned char;
    static_assert(SlabsCount < 255 - 1, "`in_use_t` can not hold SlabsCount");

    using storage_t = std::aligned_storage_t<SlabSize>;

    in_use_t    in_use_[SlabsCount] = {}; // zero initialize
    storage_t   storages_[SlabsCount];

};

using slab_allocator_t = slab_allocator_basic_t<64u, 4u>;

} // namespace dmn

