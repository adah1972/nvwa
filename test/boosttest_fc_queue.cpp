#include "nvwa/fc_queue.h"
#include <iostream>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <boost/core/demangle.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;

class Obj {
public:
    Obj() {}
    ~Obj() noexcept(false) {}
};

void swap(Obj& lhs, Obj& rhs) _NOEXCEPT;

namespace {

template <typename T>
void check_type()
{
    using test_type = nvwa::fc_queue<T>;

    BOOST_TEST_MESSAGE("Checking type "
                       << boost::core::demangle(typeid(T).name()));
    BOOST_TEST_MESSAGE("is_nothrow_constructible is "
                << std::is_nothrow_constructible<test_type>::value);
    BOOST_TEST_MESSAGE("is_nothrow_default_constructible is "
                << std::is_nothrow_default_constructible<test_type>::value);
    BOOST_TEST_MESSAGE("is_nothrow_move_constructible is "
                << std::is_nothrow_move_constructible<test_type>::value);
    BOOST_TEST_MESSAGE("is_nothrow_copy_constructible is "
                << std::is_nothrow_copy_constructible<test_type>::value);
    BOOST_TEST_MESSAGE("is_nothrow_move_assignable is "
                << std::is_nothrow_move_assignable<test_type>::value);
    BOOST_TEST_MESSAGE("is_nothrow_copy_assignable is "
                << std::is_nothrow_copy_assignable<test_type>::value);
    BOOST_TEST_MESSAGE("is_nothrow_destructible is "
                << std::is_nothrow_destructible<test_type>::value);
}

}

BOOST_AUTO_TEST_CASE(fc_queue_test)
{
    nvwa::fc_queue<int> q(4);
    BOOST_TEST_MESSAGE("sizeof fc_queue is " << sizeof q);
    BOOST_CHECK_EQUAL(q.capacity(), 4U);
    BOOST_CHECK_EQUAL(q.size(), 0U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(q.empty());
    q.push(1);
    BOOST_CHECK_EQUAL(q.size(), 1U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 1);
    BOOST_CHECK_EQUAL(q.back(), 1);
    q.push(2);
    BOOST_CHECK_EQUAL(q.size(), 2U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 1);
    BOOST_CHECK_EQUAL(q.back(), 2);
    q.push(3);
    BOOST_CHECK_EQUAL(q.size(), 3U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 1);
    BOOST_CHECK_EQUAL(q.back(), 3);
    q.push(4);
    BOOST_CHECK_EQUAL(q.size(), 4U);
    BOOST_CHECK(q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 1);
    BOOST_CHECK_EQUAL(q.back(), 4);
    q.push(5);
    BOOST_CHECK_EQUAL(q.size(), 4U);
    BOOST_CHECK(q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 2);
    BOOST_CHECK_EQUAL(q.back(), 5);
    BOOST_CHECK(!q.contains(1));
    BOOST_CHECK(q.contains(2));
    BOOST_CHECK(q.contains(3));
    BOOST_CHECK(q.contains(5));
    BOOST_CHECK(!q.contains(6));
    q.pop();
    BOOST_CHECK_EQUAL(q.size(), 3U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 3);
    BOOST_CHECK_EQUAL(q.back(), 5);
    q.pop();
    BOOST_CHECK_EQUAL(q.size(), 2U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 4);
    BOOST_CHECK_EQUAL(q.back(), 5);
    q.pop();
    BOOST_CHECK_EQUAL(q.size(), 1U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.front(), 5);
    BOOST_CHECK_EQUAL(q.back(), 5);

    nvwa::fc_queue<int> r(q);
    q.pop();
    BOOST_CHECK_EQUAL(q.size(), 0U);
    BOOST_CHECK(!q.full());
    BOOST_CHECK(q.empty());
    BOOST_CHECK(!r.full());
    BOOST_CHECK(!r.empty());
    BOOST_CHECK_EQUAL(r.front(), 5);
    BOOST_CHECK_EQUAL(r.back(), 5);
    q = std::move(r);
    BOOST_CHECK(r.empty());
    BOOST_CHECK(r.full());
    BOOST_CHECK(!q.empty());
    BOOST_CHECK(!q.full());
    BOOST_CHECK_EQUAL(q.front(), 5);
    BOOST_CHECK_EQUAL(q.back(), 5);

    nvwa::fc_queue<int> s;
    BOOST_CHECK(s.empty());
    BOOST_CHECK(s.full());
    BOOST_CHECK_EQUAL(s.capacity(), 0);
    BOOST_CHECK_EQUAL(s.size(), 0);
    s = q;
    BOOST_CHECK(!q.empty());
    BOOST_CHECK(!q.full());
    BOOST_CHECK_EQUAL(q.front(), 5);
    BOOST_CHECK_EQUAL(q.back(), 5);
    BOOST_CHECK(!s.empty());
    BOOST_CHECK(!s.full());
    BOOST_CHECK_EQUAL(s.front(), 5);
    BOOST_CHECK_EQUAL(s.back(), 5);
    BOOST_CHECK_EQUAL(s.capacity(), 4);
    BOOST_CHECK_EQUAL(s.size(), 1);
    s = nvwa::fc_queue<int>(5);
    BOOST_CHECK(s.empty());
    BOOST_CHECK(!s.full());
    BOOST_CHECK_EQUAL(s.capacity(), 5);
    BOOST_CHECK_EQUAL(s.size(), 0);
    s.push(1);
    BOOST_CHECK_EQUAL(s.front(), 1);
    BOOST_CHECK_EQUAL(s.back(), 1);
    BOOST_CHECK_EQUAL(s.size(), 1);

    check_type<int>();
    check_type<Obj>();
}
