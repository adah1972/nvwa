#include "nvwa/tree.h"
#include <ostream>
#include <sstream>
#include <string>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;
using namespace nvwa;

namespace /* unnamed */ {

template <typename Tree>
void traverse_in_order_recursively(Tree& node, std::ostream& os)
{
    if (!node->empty() && node->child(0)) {
        traverse_in_order_recursively(node->child(0), os);
    }
    os << node->value() << ' ';
    if (node->empty()) {
        return;
    }
    auto it = node->begin();
    for (++it; it != node->end(); ++it) {
        if (*it) {
            traverse_in_order_recursively(*it, os);
        }
    }
}

template <storage_policy Policy>
void test_tree()
{
    //       6
    //      / \
    //     4   7
    //    / \   \
    //   2   5   9
    //  / \     / \
    // 1   3   8   10
    auto root =
        create_tree<Policy>(6,
            create_tree<Policy>(4,
                create_tree<Policy>(2,
                    create_tree<Policy>(1),
                    create_tree<Policy>(3)),
                create_tree<Policy>(5)),
            create_tree<Policy>(7,
                tree<int, Policy>::null(),
                create_tree<Policy>(9,
                    create_tree<Policy>(8),
                    create_tree<Policy>(10))));

    std::ostringstream oss;
    for (auto& node : traverse_breadth_first(*root)) {
        oss << node.value() << ' ';
    }
    BOOST_TEST_MESSAGE("Breadth-first traversal: " << oss.str());
    BOOST_CHECK_EQUAL(oss.str(), "6 4 7 2 5 9 1 3 8 10 ");

    oss.str("");
    for (auto& node : traverse_depth_first(*root)) {
        oss << node.value() << ' ';
    }
    BOOST_TEST_MESSAGE("Depth-first traversal:   " << oss.str());
    BOOST_CHECK_EQUAL(oss.str(), "6 4 2 1 3 5 7 9 8 10 ");

    oss.str("");
    for (auto& node : traverse_in_order(*root)) {
        oss << node.value() << ' ';
        node.value() *= 2;
    }
    BOOST_TEST_MESSAGE("In-order traversal:      " << oss.str());
    BOOST_CHECK_EQUAL(oss.str(), "1 2 3 4 5 6 7 8 9 10 ");

    oss.str("");
    traverse_in_order_recursively(root, oss);
    BOOST_TEST_MESSAGE("All values are doubled:  " << oss.str());
    BOOST_CHECK_EQUAL(oss.str(), "2 4 6 8 10 12 14 16 18 20 ");

    oss.str("");
    auto child = std::move(root->child(0)->child(0)->child(1));
    traverse_in_order_recursively(root, oss);
    BOOST_TEST_MESSAGE("One leaf is dropped:     " << oss.str());
    BOOST_CHECK_EQUAL(oss.str(), "2 4 8 10 12 14 16 18 20 ");
}

} /* unnamed namespace */

BOOST_AUTO_TEST_CASE(tree_test)
{
    BOOST_TEST_MESSAGE("Testing with unique ownership");
    test_tree<storage_policy::unique>();
    BOOST_TEST_MESSAGE("Testing with shared ownership");
    test_tree<storage_policy::shared>();
}
