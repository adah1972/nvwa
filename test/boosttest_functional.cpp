#include "nvwa/functional.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;

namespace /* unnamed */ {

auto const f = [](int v) { return v - 1.f; };
auto const g = [](float v) { return static_cast<int>(v); };

constexpr int increase(int n)
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

constexpr int sqr(int x)
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

template <typename T>
constexpr auto sum(T x)
{
    return x;
}

template <typename T1, typename T2, typename... Targ>
constexpr auto sum(T1 x, T2 y, Targ... args)
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
    if (data.has_value())
        os << "just " << data.value();
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

    constexpr nvwa::optional<int> nothing;
    constexpr auto r1 = nvwa::apply(increase, nothing);
    constexpr auto r2 = nvwa::apply(increase, nvwa::make_optional(41));
    BOOST_CHECK(!r1.has_value());
    BOOST_CHECK(r2.has_value() && r2.value() == 42);
    auto r3 = r2;
    BOOST_CHECK(r2.has_value() && r2.value() == 42);
    BOOST_CHECK(r3.has_value() && r3.value() == 42);
    auto r4 = r3;
    BOOST_CHECK(r4.has_value() && r4.value() == 42);
    BOOST_CHECK_EQUAL(r4.value_or(-1), 42);
    r3 = 43;
    BOOST_CHECK_EQUAL(r3.value_or(-1), 43);
    r3.emplace(44);
    BOOST_CHECK_EQUAL(r3.value_or(-1), 44);
    swap(r3, r4);
    BOOST_CHECK_EQUAL(r3.value_or(-1), 42);
    BOOST_CHECK_EQUAL(r4.value_or(-1), 44);
    r3.reset();
    BOOST_CHECK_EQUAL(r3.value_or(-1), -1);
    BOOST_CHECK_EQUAL(nvwa::make_optional(42).value_or(-1), 42);
    BOOST_CHECK_EQUAL(nvwa::optional<int>().value_or(-1), -1);
    auto const inc_opt = nvwa::lift_optional(increase);
    BOOST_CHECK([](nvwa::optional<int> x) { return !x.has_value(); }
                  (inc_opt(r1)));
    BOOST_CHECK([](nvwa::optional<int> x)
                {
                    return x.has_value() && x.value() == 43;
                }(inc_opt(r2)));

    auto p1 = nvwa::make_optional(std::make_unique<int>(45));
    BOOST_CHECK(p1.has_value());
    BOOST_CHECK(p1->get() != nullptr);
    BOOST_CHECK_EQUAL(**p1, 45);
    auto p2 = std::move(p1);
    BOOST_CHECK(p2.has_value());
    BOOST_CHECK(p2->get() != nullptr);
    BOOST_CHECK_EQUAL(**p2, 45);
    BOOST_CHECK(!p1.has_value());
    BOOST_CHECK(noexcept(swap(std::declval<std::unique_ptr<int>&>(),
                              std::declval<std::unique_ptr<int>&>())));
    BOOST_CHECK(noexcept(swap(p1, p2)));
    swap(p1, p2);
    BOOST_CHECK(p1->get() != nullptr && **p1 == 45);
    BOOST_CHECK(!p2.has_value());

    using namespace nvwa;
    BOOST_CHECK(noexcept(
        detail::adl_swap(std::declval<int&>(), std::declval<int&>())));
    BOOST_CHECK(noexcept(int(std::move(std::declval<int&&>()))));
    BOOST_CHECK(std::is_trivially_destructible<int>::value ||
                std::is_nothrow_destructible<int>::value);
    BOOST_CHECK(noexcept(r3.swap(r4)));

    assert((nvwa::detail::can_reserve<std::vector<int>,
                                      std::list<int>>::value));
    auto id = nvwa::compose();
    int a[] = {1, 2, 3, 4, 5};
    BOOST_CHECK_EQUAL(SumList()(a), 15);
    std::vector<int> v = nvwa::fmap(id, a);
    std::ostringstream oss;  // boost cannot find my operator<< via ADL
    oss << v;
    BOOST_TEST_MESSAGE("Test vector is " << oss.str());
    oss.str(std::string());
    oss << nvwa::apply(SquareList(), nvwa::make_optional(v));
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
    BOOST_CHECK_EQUAL(nvwa::compose(g, f, id)(1), 0);

    (void)sum<int, int, int, int, int>;  // GCC requires this instantiation
    auto sum_and_square = nvwa::compose(sqr, sum<int, int, int, int, int>);
    BOOST_CHECK_EQUAL(sum_and_square(1, 2, 3, 4, 5), 225);
    auto square_and_sum_2 =
        nvwa::compose(nvwa::wrap_args_as_pair<int, int>(std::plus<int>()),
                      [](auto&& x) { return nvwa::fmap(sqr, x); });
    auto square_and_sum_4 = nvwa::compose(
        nvwa::wrap_args_as_tuple<std::tuple<int, int, int, int>>(
            sum<int, int, int, int>),
        [](auto&& x) { return nvwa::fmap(sqr, x); });
    auto square_and_sum = nvwa::compose(
        [](auto&& x) { return nvwa::reduce(std::plus<int>(), x, 0); },
        [](auto&& x) { return nvwa::fmap(sqr, x); });
    BOOST_CHECK_EQUAL(square_and_sum_2(std::make_pair(3, 4)), 25);
    BOOST_CHECK_EQUAL(square_and_sum_4(std::make_tuple(1, 2, 3, 4)), 30);
    auto t = std::make_tuple(1, 2, 3, 4, 5);
    BOOST_CHECK_EQUAL(square_and_sum(t), 55);
    BOOST_CHECK_EQUAL(
        nvwa::apply(sum<int, int, int, int, int>, fmap(sqr, t)), 55);

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
