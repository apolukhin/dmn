#include "impl/circular_iterator.hpp"
#include "impl/lazy_array.hpp"
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

BOOST_AUTO_TEST_CASE(lazy_array_basic_test) {
    struct non_default_constr {
        int& destructions_count;
        non_default_constr(int, int, int& destructions_count)
            : destructions_count(destructions_count)
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
        int& destructions_count;

        base(int& destructions_count)
            : destructions_count(destructions_count)
        {}

        virtual bool is_ok() const { return false; }
        virtual ~base() {
            ++destructions_count;
        }
    };

    struct derived: base {
        using base::base;
        virtual bool is_ok() const override { return true; }
        virtual ~derived() {
            ++destructions_count;
        }
    };

    struct derived2: base {
        using base::base;
        bool b = true;

        virtual bool is_ok() const override { return b; }
        virtual ~derived2() {
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
