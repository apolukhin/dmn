#include "impl/circular_iterator.hpp"
#include "impl/lazy_array.hpp"
#include "impl/net/slab_allocator.hpp"

#include <vector>
#include <random>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(circular_iterator_test) {
    using it_t = dmn::circular_iterator<std::vector<int>>;

    std::vector<int> vec;
    vec.resize(1024, 0);
    std::for_each(it_t(vec, 0, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(vec == std::vector<int>(1024, 1));

    std::for_each(it_t(vec, 500, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(vec == std::vector<int>(1024, 2));

    std::for_each(it_t(vec, 1024, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(vec == std::vector<int>(1024, 3));

    std::for_each(it_t(vec, 10024, 1024), it_t{}, [](auto& v){ ++v;});
    BOOST_TEST(vec == std::vector<int>(1024, 4));

    std::for_each(it_t(vec, 1023, 2), it_t{}, [](auto& v){ ++v;});
    std::vector<int> ethalon(1, 5);
    ethalon.resize(1024-1, 4);
    ethalon.resize(1024, 5);
    BOOST_TEST(vec == ethalon);
}

BOOST_AUTO_TEST_CASE(slab_allocator_test) {
    dmn::slab_allocator_t a;

    // asio's memory usage on Linux is about 200 bytes
    BOOST_TEST(a.slabs_count * a.slab_size >= 200);

    for (unsigned i = 0; i < 10; ++i) {
        auto* p = a.allocate(200);
        BOOST_TEST(p);
        a.deallocate(p);
    }

    for (unsigned i = 0; i < 10; ++i) {
        auto* p0 = a.allocate(173);
        auto* p1 = a.allocate(10);
        BOOST_TEST(p0);
        BOOST_TEST(p1);
        a.deallocate(p1);
        a.deallocate(p0);
    }

    {
        auto* p1 = a.allocate(10);
        BOOST_TEST(p1);
        for (unsigned i = 0; i < 10; ++i) {
            auto* p0 = a.allocate(173);
            BOOST_TEST(p0);
            a.deallocate(p0);
        }
        a.deallocate(p1);
    }

    {
        auto* p1 = a.allocate(10);
        BOOST_TEST(p1);
        auto* p3 = a.allocate(64);
        BOOST_TEST(p3);
        for (unsigned i = 0; i < 10; ++i) {
            auto* p0 = a.allocate(128);
            BOOST_TEST(p0);
            a.deallocate(p0);
        }
        a.deallocate(p1);
        a.deallocate(p3);
    }

    {
        auto* p1 = a.allocate(128);
        BOOST_TEST(p1);
        for (unsigned i = 0; i < 10; ++i) {
            auto* p0 = a.allocate(32);
            auto* p3 = a.allocate(64);
            BOOST_TEST(p0);
            BOOST_TEST(p3);
            a.deallocate(p0);
            a.deallocate(p3);
        }
        a.deallocate(p1);
    }

    for (unsigned i = 1; i < 10; ++i) {
        auto* p0 = a.allocate(i);
        auto* p1 = a.allocate(i * 3);
        BOOST_TEST(p0);
        BOOST_TEST(p1);
        a.deallocate(p0);
        a.deallocate(p1);
    }
}

BOOST_AUTO_TEST_CASE(slab_allocator_tortue) {
    dmn::slab_allocator_t a;
    std::vector<void*> allocated;
    allocated.reserve(a.slabs_count);
    std::default_random_engine dre{};

    for (unsigned chunks = 1; chunks < a.slabs_count; ++chunks) {
        for (unsigned i = 0; i < 10; ++i) {
            for (unsigned j = 0; j < a.slabs_count / chunks; ++j) {
                auto* p = a.allocate(a.slab_size * chunks);
                BOOST_TEST(p);
                allocated.push_back(p);
            }

            std::shuffle(allocated.begin(), allocated.end(), dre);

            for (unsigned j = 0; j < a.slabs_count / chunks; ++j) {
                auto* p = allocated.back();
                allocated.pop_back();
                BOOST_TEST(p);
                a.deallocate(p);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(slab_allocator_fragmentation) {
    dmn::slab_allocator_t a;
    std::vector<void*> allocated;
    allocated.reserve(a.slabs_count);
    std::default_random_engine dre{};

    for (unsigned j = 0; j < a.slabs_count; ++j) {
        auto* p = a.allocate(a.slab_size);
        BOOST_TEST(p);
        allocated.push_back(p);
    }

    for (unsigned i = 0; i < 40; ++i) {
        std::shuffle(allocated.begin(), allocated.end(), dre);

        auto* p = allocated.back();
        a.deallocate(p);
        allocated.back() = a.allocate(a.slab_size);
        BOOST_TEST(allocated.back());
    }

    for (void* p: allocated) {
        a.deallocate(p);
    }
}

BOOST_AUTO_TEST_CASE(slab_allocator_test_zeros) {
    dmn::slab_allocator_t a;
    for (unsigned i = 0; i < 20; ++i) {
        BOOST_TEST(a.allocate(0) == nullptr);
    }

    a.deallocate(nullptr);
}

BOOST_AUTO_TEST_CASE(slab_allocator_test_huge_block) {
    dmn::slab_allocator_t a;
    for (unsigned i = 0; i < 10; ++i) {
        auto* p = a.allocate(256);
        BOOST_TEST(p);
        a.deallocate(p);
    }
}

BOOST_AUTO_TEST_CASE(lazy_array_basic_test) {
    struct non_default_constr {
        non_default_constr(const non_default_constr&) = delete;
        non_default_constr& operator=(const non_default_constr&) = delete;

        int& destructions_count;

        non_default_constr(int, int, int& destruc_count)
            : destructions_count(destruc_count)
        {}
        ~non_default_constr() {
            ++ destructions_count;
        }
    };

    int destructions_count = 0;
    {

        dmn::lazy_array<non_default_constr> la;
        la.init(3);
        la.inplace_construct(0, 0, 0, destructions_count);
        la.inplace_construct(1, 0, 0, destructions_count);
        la.inplace_construct(2, 0, 0, destructions_count);

        for (auto& v: la) {
            BOOST_TEST(v.destructions_count == 0);
        }

        const auto& cla = la;
        for (auto& v: cla) {
            BOOST_TEST(v.destructions_count == 0);
        }

    }
    BOOST_TEST(destructions_count == 3);
}

BOOST_AUTO_TEST_CASE(lazy_array_derived_test) {
    struct base {
        base(const base&) = delete;
        base& operator=(const base&) = delete;

        int& destructions_count;

        base(int& destruc_count)
            : destructions_count(destruc_count)
        {}

        virtual bool is_ok() const { return false; }
        virtual ~base() {
            ++destructions_count;
        }
    };

    struct derived: base {
        using base::base;
        bool is_ok() const override { return true; }
        ~derived() override {
            ++destructions_count;
        }
    };

    struct derived2: base {
        using base::base;
        bool b = true;

         bool is_ok() const override { return b; }
        ~derived2() override {
            ++destructions_count;
        }
    };

    int destructions_count = 0;
    {
        dmn::lazy_array<base, sizeof(derived2)> la;
        la.init(4);
        la.inplace_construct_derived<derived>(0, destructions_count);
        la.inplace_construct_derived<derived2>(1, destructions_count);
        la.inplace_construct_derived<derived>(2, destructions_count);
        la.inplace_construct_derived<derived2>(3, destructions_count);

        for (auto& v: la) {
            BOOST_TEST(v.is_ok());
        }
    }
    BOOST_TEST(destructions_count == 8);
}
