#include "nvwa/fixed_mem_pool.h"
#include <iostream>
#include <utility>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif

class Obj {
public:
    Obj() {}
private:
    char a[12];
    DECLARE_FIXED_MEM_POOL(Obj)
};

#ifdef __clang__
#pragma GCC diagnostic pop
#endif

BOOST_AUTO_TEST_CASE(fixed_mem_test)
{
    BOOST_REQUIRE(nvwa::fixed_mem_pool<Obj>::initialize(4));
    Obj* p1;
    Obj* p2;
    Obj* p3;
    Obj* p4;
    p1 = new Obj();
    BOOST_CHECK_EQUAL(nvwa::fixed_mem_pool<Obj>::deinitialize(), 1);
    p2 = new Obj();
    BOOST_CHECK_EQUAL(nvwa::fixed_mem_pool<Obj>::deinitialize(), 2);
    p3 = new Obj();
    BOOST_CHECK_EQUAL(nvwa::fixed_mem_pool<Obj>::deinitialize(), 3);
    p4 = new Obj();
    BOOST_CHECK_EQUAL(nvwa::fixed_mem_pool<Obj>::deinitialize(), 4);
    BOOST_REQUIRE_THROW(new Obj(), std::bad_alloc);
    delete p1;
    delete p2;
    delete p3;
    delete p4;
    BOOST_CHECK_EQUAL(nvwa::fixed_mem_pool<Obj>::deinitialize(), 0);
}
