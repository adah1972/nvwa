#include "nvwa/split.h"
#include <stddef.h>
#include <array>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>
#include <boost/test/unit_test.hpp>
#include "nvwa/c++_features.h"

#if HAVE_CXX20_RANGES
#include <charconv>
#include <ranges>
#endif

constexpr std::string_view str{
    "&grant_type=client_credential&appid=&secret=APPSECRET"};

std::vector<std::string> split_result_expected{
    "",
    "grant_type=client_credential",
    "appid=",
    "secret=APPSECRET"
};

BOOST_AUTO_TEST_CASE(split_test)
{
    using namespace std::literals;

    constexpr auto result = nvwa::split(str, '&');
    constexpr auto begin = result.begin();
    (void)begin;  // Only test its constexpr-ness
    constexpr auto end = result.end();
    auto result_s = result.to_vector();
    auto result_sv = result.to_vector_sv();
    BOOST_TEST(result_s == split_result_expected);
    BOOST_TEST_REQUIRE(result_s.size() == result_sv.size());
    {
        auto it = result_sv.begin();
        for (auto& s : result_s) {
            BOOST_CHECK_EQUAL(s, *it);
            ++it;
        }
    }
    size_t i = 0;
    auto it = result.begin();
    for (; it != end && i < result_s.size(); ++it) {
        BOOST_TEST(*it ==result_s[i]);
        if (!result_s[i].empty()) {
            BOOST_TEST(nvwa::split(*it, "="sv).to_vector_sv().size() == 2U);
        }
        ++i;
    }
    BOOST_CHECK(it == end);

#if HAVE_CXX20_RANGES
    static_assert(std::ranges::forward_range<decltype(result)>);
    static_assert(std::ranges::view<std::remove_const_t<decltype(result)>>);

#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L
    auto ip = "127.0.0.1"s;
    auto parts = nvwa::split(ip, "."sv) |
                 std::views::transform([](std::string_view part) {
                     unsigned char result{};
                     auto [ptr, ec] = std::from_chars(
                         part.data(), part.data() + part.size(), result);
                     if (ec != std::errc{}) {
                         throw std::system_error(std::make_error_code(ec));
                     }
                     return result;
                 });
    std::array expected_result{127, 0, 0, 1};
    i = 0;
    for (int n : parts) {
        if (i >= expected_result.size()) {
            BOOST_FAIL("Too many parts");
        }
        BOOST_TEST(n == expected_result[i++]);
    }
    BOOST_TEST(i == expected_result.size());
#endif
#endif
}
