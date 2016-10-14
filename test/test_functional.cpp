#include "nvwa/functional.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <utility>
#include <vector>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;

namespace /* unnamed */ {

auto const f = [](int v) { return v - 1.f; };
auto const g = [](float v) { return static_cast<int>(v); };

int increase(int n)
{
    return n + 1;
}

int fact(std::function<int(int)> f, int v)
{
    if (v == 0)
        return 1;
    else
        return v * f(v - 1);
}

int sqr(int x)
{
    return x * x;
}

template <typename Number>
struct Square {
    Number operator()(Number value) const
    {
        return value * value;
    }
};

struct SquareList {
    template <typename Data>
    auto operator()(const Data& a) const
    {
        return nvwa::fmap<std::list>(sqr, a);
    }
};

struct SumList {
    template <typename Data>
    auto operator()(const Data& a) const
    {
        return nvwa::reduce(std::plus<int>(), a);
    }
};

template <typename T1, typename T2>
auto sum(T1 x, T2 y)
{
    return x + y;
}

template <typename T1, typename T2, typename... Targ>
auto sum(T1 x, T2 y, Targ... args)
{
    return sum(x + y, args...);
}

template <typename T>
std::ostream& print_with_space(std::ostream& os, const T& data)
{
    os << data << ' ';
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& data)
{
    os << "[ ";
    nvwa::reduce(print_with_space<T>, data, os);;
    os << ']';
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& data)
{
    os << "( ";
    std::copy(data.begin(), data.end(),
              std::ostream_iterator<T>(os, " "));
    os << ')';
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const nvwa::optional<T>& data)
{
    if (data.is_valid())
        os << "just " << data.cref();
    else
        os << "invalid";
    return os;
}

struct Obj {
    static int created_;
    static int copied_;
    static int moved_;

    int value;
    Obj() : Obj(0)
    {
    }
    explicit Obj(int n) : value(n)
    {
        if (value != 99) {
            BOOST_TEST_MESSAGE("Obj() :" << value);
            ++created_;
        }
    }
    Obj(const Obj& rhs) : value(rhs.value)
    {
        BOOST_TEST_MESSAGE("Obj(const Obj&) :" << value);
        ++copied_;
    }
    Obj(Obj&& rhs) : value(rhs.value)
    {
        BOOST_TEST_MESSAGE("Obj(Obj&&) :" << value);
        rhs.value = -2;
        ++moved_;
    }
    ~Obj()
    {
        assert(value != -1);
        if (value == 99) {
            BOOST_TEST_MESSAGE("Created: " << created_);
            BOOST_TEST_MESSAGE("Copied:  " << copied_);
            BOOST_TEST_MESSAGE("Moved:   " << moved_);
        } else if (value != -2) {
            BOOST_TEST_MESSAGE("~Obj() :" << value);
        }
        value = -1;
    }
};

int Obj::created_ = 0;
int Obj::copied_ = 0;
int Obj::moved_ = 0;

void test_curry(Obj& o1, Obj o2, Obj&& o3)
{
    BOOST_TEST_MESSAGE("Test currying: " << o1.value << o2.value
                                         << o3.value);
}

void test_out3(std::ostream& os, std::string a, const std::string& b,
               const std::string& c)
{
    os << a << b << c;
}

} /* unnamed namespace */

BOOST_AUTO_TEST_CASE(functional_test)
{
    Obj guard(99);  // special guard
    nvwa::optional<int> nothing;
    auto r1 = fmap(increase, nothing);
    auto r2 = fmap(increase, nvwa::make_optional(41));
    BOOST_CHECK(!r1.is_valid());
    BOOST_CHECK(r2.is_valid() && r2.value() == 42);
    auto const inc_opt = nvwa::lift_optional<int>::make(increase);
    BOOST_CHECK([](nvwa::optional<int> x) { return !x.is_valid(); }
                  (inc_opt(r1)));
    BOOST_CHECK([](nvwa::optional<int> x)
                {
                    return x.is_valid() && x.value() == 43;
                }(inc_opt(r2)));
    assert((nvwa::detail::can_reserve<std::vector<int>,
                                      std::list<int>>::value));
    std::vector<int> v{1, 2, 3, 4, 5};
    std::ostringstream oss;  // boost cannot find my operator<< via ADL
    oss << v;
    BOOST_TEST_MESSAGE("Test vector is " << oss.str());
    oss.str(std::string());
    oss << fmap(SquareList(), nvwa::make_optional(v));
    BOOST_TEST_MESSAGE("Square list is " << oss.str());
    BOOST_CHECK_EQUAL(SumList()(v), 15);
    auto squared_sum = nvwa::compose(SumList(), SquareList());
    BOOST_CHECK_EQUAL(squared_sum(v), 55);
    using nvwa::pipeline;
    BOOST_CHECK_EQUAL(pipeline(v, squared_sum), 55);
    BOOST_CHECK_EQUAL(pipeline(v, SquareList(), SumList()), 55);
    BOOST_CHECK_EQUAL(
        pipeline(v,
                 [](const auto& x)
                 {
                     return nvwa::fmap(Square<int>(), x);
                 },
                 [](const auto& x)
                 {
                     return nvwa::reduce(std::plus<int>(), x);
                 }),
        55);

    {
        auto const id = nvwa::compose();
        Obj obj = id(Obj());
    }

    auto inc = nvwa::compose(increase);
    BOOST_CHECK_EQUAL(nvwa::compose(inc, inc, inc)(2), 5);
    BOOST_CHECK_EQUAL(nvwa::pipeline(v,
                                     [](const std::vector<int>& v)
                                     {
                                         return nvwa::reduce(
                                             std::plus<int>(), v);
                                     },
                                     inc, inc),
                      17);
    auto plus_1 = [](int x) { return x + 1; };
    auto mult_2 = [](int x) { return x * 2; };
    BOOST_CHECK_EQUAL(nvwa::compose(plus_1, mult_2)(1), 3);
    auto id = nvwa::compose();
    BOOST_CHECK_EQUAL(nvwa::compose(g, f, id)(1), 0);

    (void)sum<int, int, int, int, int>;  // GCC requires this instantiation
    auto sum_and_square = nvwa::compose(sqr, sum<int, int, int, int, int>);
    BOOST_CHECK_EQUAL(sum_and_square(1, 2, 3, 4, 5), 225);

    using std::function;
    typedef function<int(int)> Func;
    typedef function<Func(Func)> FuncFunc;

    FuncFunc almost_fac = [](Func f)
    {
        return Func([f](int n)
        {
            if (n <= 1)
                return 1;
            else
                return n * f(n - 1);
        });
    };
    FuncFunc almost_fac2 = nvwa::make_curry(fact);
    auto const fac1 = nvwa::fix_simple(almost_fac);
    auto const fac2 = nvwa::fix_curry(almost_fac);
    auto const fac3 = nvwa::fix_simple<int, int>(fact);
    auto const fac4 = nvwa::fix_curry(almost_fac2);
    BOOST_CHECK_EQUAL(fac1(5), 120);
    BOOST_CHECK_EQUAL(fac2(5), 120);
    BOOST_CHECK_EQUAL(fac3(5), 120);
    BOOST_CHECK_EQUAL(fac4(5), 120);

    Obj obj1(1);
    auto const middle = nvwa::make_curry(test_curry)(obj1)(Obj(2));
    middle(Obj(3));
    middle(Obj(3));
    oss.str(std::string());
    nvwa::make_curry(test_out3)(oss)("Hello ")("functional ")("world!");
    BOOST_CHECK_EQUAL(oss.str(), "Hello functional world!");
}
