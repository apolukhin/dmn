#pragma once
#include <type_traits>
#include "slab_allocator.hpp"

namespace dmn {

// Wrapper class template for handler objects to allow handler memory
// allocation to be customised. Calls to operator() are forwarded to the
// encapsulated handler.
template <typename Handler>
class slab_alloc_handler {
public:
    template <class H>
    inline slab_alloc_handler(slab_allocator_t& a, H&& h)
        : allocator_(a)
        , handler_(std::forward<H>(h))
    {}

    template <typename... Args>
    inline void operator()(Args&&... args) {
        handler_(std::forward<Args>(args)...);
    }

    inline friend void* asio_handler_allocate(std::size_t size, slab_alloc_handler<Handler>* this_handler) {
        return this_handler->allocator_.allocate(size);
    }

    inline friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, slab_alloc_handler<Handler>* this_handler) noexcept {
        this_handler->allocator_.deallocate(pointer);
    }

private:
    slab_allocator_t& allocator_;
    Handler handler_;
};

// Helper function to wrap a handler object to add custom allocation.
template <typename Handler>
inline slab_alloc_handler<typename std::remove_reference<Handler>::type> make_slab_alloc_handler(slab_allocator_t& a, Handler&& h) {
    return {a, std::forward<Handler>(h)};
}

#define DMN_USE_SLAB(class_name)                                                                \
    void* allocate(std::size_t size) { return this_.slab_.allocate(size); }                     \
    void deallocate(void* pointer) noexcept { this_.slab_.deallocate(pointer); }                \
    inline friend void* asio_handler_allocate(std::size_t size, class_name* this_handler) {     \
        return this_handler->allocate(size);                                                    \
    }                                                                                           \
    inline friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,             \
            class_name* this_handler) noexcept                                                  \
    {                                                                                           \
        this_handler->deallocate(pointer);                                                      \
    }                                                                                           \
/**/

} // namespace dmn


