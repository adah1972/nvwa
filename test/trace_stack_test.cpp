#include "nvwa/trace_stack.h"
#include <utility>
#include <boost/test/unit_test.hpp>
#include "nvwa/c++_features.h"

#if HAVE_CXX20_RANGES

#include <ranges>

BOOST_AUTO_TEST_CASE(trace_stack_concept_check)
{
    nvwa::trace_stack<int> tst;
    BOOST_CHECK(!std::ranges::range<nvwa::trace_stack<int>>);
    BOOST_CHECK(std::ranges::range<decltype(tst.get_popped())>);
    BOOST_CHECK(std::ranges::borrowed_range<decltype(tst.get_popped())>);
    BOOST_CHECK(std::ranges::view<decltype(tst.get_popped())>);
}

#endif

BOOST_AUTO_TEST_CASE(trace_stack_test)
{
    nvwa::trace_stack<int> tst;
    BOOST_TEST(tst.size() == 0U);
    BOOST_TEST(tst.empty());
    tst.push(1);
    BOOST_TEST(!tst.empty());
    tst.push(2);
    tst.push(3);
    BOOST_TEST(tst.size() == 3U);
    BOOST_TEST(tst.top() == 3);
    tst.pop();
    BOOST_TEST(tst.top() == 2);
    tst.pop();
    BOOST_TEST(tst.top() == 1);
    tst.pop();
    BOOST_TEST(tst.empty());

    BOOST_REQUIRE(tst.get_popped().size() == 3U);
    int sum = 0;
    for (int n : tst.get_popped()) {
        sum += n;
    }
    BOOST_TEST(sum == 6);

    auto tst2 = std::move(tst);
    BOOST_TEST(tst.empty());
    tst2.push(4);
    BOOST_TEST(tst2.top() == 4);
    BOOST_TEST(tst2.get_popped().empty());
}

