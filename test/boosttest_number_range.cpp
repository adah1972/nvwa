#include "nvwa/number_range.h"
#include <functional>
#include <boost/test/unit_test.hpp>
#include "nvwa/functional.h"

BOOST_AUTO_TEST_CASE(number_range_test)
{
    BOOST_CHECK_EQUAL(
        nvwa::reduce(std::plus<>(), nvwa::number_range(1, 101)), 5050);
    BOOST_CHECK_EQUAL(
        nvwa::reduce(std::plus<>(), nvwa::number_range(2.0, 201.0, 2.0)),
        10100.0);
    BOOST_CHECK_EQUAL(
        nvwa::reduce(std::plus<>(), nvwa::number_range(2.0, 200.0, 2.0)),
        9900.0);
}
