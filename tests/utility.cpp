#include "impl/circular_iterator.hpp"
#include "impl/net/slab_allocator.hpp"

#include <vector>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(circular_iterator_test) {
    using it_t = dmn::circular_iterator<std::vector<int>>;

    std::vector<int> v;
    v.resize(1024, 0);
    std::for_each(it_t(v, 0, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(v == std::vector<int>(1024, 1));

    std::for_each(it_t(v, 500, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(v == std::vector<int>(1024, 2));

    std::for_each(it_t(v, 1024, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(v == std::vector<int>(1024, 3));

    std::for_each(it_t(v, 10024, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(v == std::vector<int>(1024, 4));

    std::for_each(it_t(v, 1023, 2), it_t{}, [](auto& v){ ++v;});
    std::vector<int> ethalon(1, 5);
    ethalon.resize(1024-1, 4);
    ethalon.resize(1024, 5);
    BOOST_TEST(v == ethalon);
}

BOOST_AUTO_TEST_CASE(slab_allocator_test) {
    dmn::slab_allocator_t a;
    for (unsigned i = 0; i < 10; ++i) {
        auto* p = a.allocate(200);
        BOOST_TEST(p);
        a.deallocate(p);
    }

    for (unsigned i = 0; i < 10; ++i) {
        auto* p0 = a.allocate(200);
        auto* p1 = a.allocate(200);
        BOOST_TEST(p0);
        BOOST_TEST(p1);
        a.deallocate(p1);
        a.deallocate(p0);
    }

    for (unsigned i = 0; i < 10; ++i) {
        auto* p0 = a.allocate(i * 10);
        auto* p1 = a.allocate(i * 5);
        BOOST_TEST(p0);
        BOOST_TEST(p1);
        a.deallocate(p0);
        a.deallocate(p1);
    }
}
