#include "nvwa/file_line_reader.h"
#include "nvwa/istream_line_reader.h"
#include "nvwa/mmap_byte_reader.h"
#include "nvwa/mmap_line_reader.h"
#include "nvwa/mmap_line_view.h"
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <boost/test/unit_test.hpp>

const char* FILE1 = "../LICENCE";
const char* FILE2 = "../README";

std::vector<std::string> read_file(const char* filename)
{
    std::vector<std::string> result;
    std::ifstream ifs(filename);
    BOOST_REQUIRE(ifs);
    nvwa::istream_line_reader reader{ifs};
    std::copy(reader.begin(), reader.end(), std::back_inserter(result));
    return result;
}

std::string convert_line_content(const std::vector<std::string>& lines)
{
    std::string result;
    for (auto&& line : lines) {
        result += line;
        result += '\n';
    }
    return result;
}

const std::vector<std::string>& get_line_content()
{
    static auto result = read_file(FILE1);
    return result;
}

const std::string& get_byte_content()
{
    static auto result = convert_line_content(get_line_content());
    return result;
}

template <typename T>
void test_iterator(T& reader1, T& reader2)
{
    auto it1 = reader1.begin();
    auto it2 = reader2.begin();
    BOOST_CHECK(it1 != it2);
    auto it3 = it1;
    BOOST_CHECK(it1 == it3);
    ++it1;
    BOOST_CHECK(it1 != it3);
    BOOST_CHECK(reader1.end() == reader1.end());
}

BOOST_AUTO_TEST_CASE(file_line_reader_test)
{
    FILE* fp1 = fopen(FILE1, "r");
    FILE* fp2 = fopen(FILE2, "r");
    BOOST_REQUIRE(fp1 && fp2);
    {
        nvwa::file_line_reader reader1{fp1};
        nvwa::file_line_reader reader2{fp2};
        test_iterator(reader1, reader2);
    }
    fclose(fp1);
    fclose(fp2);

    FILE* fp = fopen(FILE1, "r");
    BOOST_REQUIRE(fp);
    nvwa::file_line_reader reader{fp};
    std::vector<std::string> file_content;
    for (const char* line : reader) {
        file_content.emplace_back(line);
    }
    BOOST_REQUIRE(file_content.size() == get_line_content().size());
    BOOST_CHECK(std::equal(file_content.begin(), file_content.end(),
                           get_line_content().begin()));
}

BOOST_AUTO_TEST_CASE(mmap_byte_reader_test)
{
    {
        nvwa::mmap_byte_reader reader1{FILE1};
        nvwa::mmap_byte_reader reader2{FILE2};
        test_iterator(reader1, reader2);
    }

    std::string file_content;
    nvwa::mmap_byte_reader reader{FILE1};
    std::copy(reader.begin(), reader.end(),
              std::back_inserter(file_content));
    BOOST_REQUIRE(file_content.size() == get_byte_content().size());
    BOOST_CHECK(std::equal(file_content.begin(), file_content.end(),
                           get_byte_content().begin()));
}

BOOST_AUTO_TEST_CASE(mmap_line_reader_test)
{
    {
        nvwa::mmap_line_reader reader1{FILE1};
        nvwa::mmap_line_reader reader2{FILE2};
        test_iterator(reader1, reader2);
    }

    std::vector<std::string> file_content;
    nvwa::mmap_line_reader reader{FILE1};
    std::copy(reader.begin(), reader.end(),
              std::back_inserter(file_content));
    BOOST_REQUIRE(file_content.size() == get_line_content().size());
    BOOST_CHECK(std::equal(file_content.begin(), file_content.end(),
                           get_line_content().begin()));
}

BOOST_AUTO_TEST_CASE(mmap_line_view_test)
{
    {
        nvwa::mmap_line_view reader1{FILE1};
        nvwa::mmap_line_view reader2{FILE2};
        test_iterator(reader1, reader2);
    }

    std::vector<std::string_view> file_content;
    nvwa::mmap_line_view reader{FILE1};
    std::copy(reader.begin(), reader.end(),
              std::back_inserter(file_content));
    BOOST_REQUIRE(file_content.size() == get_line_content().size());
    BOOST_CHECK(std::equal(file_content.begin(), file_content.end(),
                           get_line_content().begin()));
}
