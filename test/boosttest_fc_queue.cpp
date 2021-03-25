#include "nvwa/fc_queue.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <boost/core/demangle.hpp>
#include <boost/test/unit_test.hpp>
#include "nvwa/pctimer.h"

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
                       << boost::core::demangle(typeid(test_type).name()));
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

const int LOOPS = 10'000'000;
std::atomic<bool> parallel_test_failed = false;

void add_to_queue(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        while (q.full()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (parallel_test_failed) {
                return;
            }
            ++stop_count;
        }
        q.push(i);
    }
    BOOST_TEST_MESSAGE(stop_count << " stops during enqueueing");
}

void read_and_check_queue(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        while (q.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ++stop_count;
        }
        if (i != q.front()) {
            BOOST_ERROR("Failure on " << i << "th read: "
                        "Expected " << i << ", got " << q.front());
            parallel_test_failed = true;
            break;
        }
        q.pop();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    BOOST_TEST_MESSAGE(stop_count << " stops during dequeueing");
}

void add_to_queue2(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        while (!q.write(i)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (parallel_test_failed) {
                return;
            }
            ++stop_count;
        }
    }
    BOOST_TEST_MESSAGE(stop_count << " stops during enqueueing");
}

void read_and_check_queue2(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        int value;
        while (!q.read(value)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ++stop_count;
        }
        if (i != value) {
            BOOST_ERROR("Failure on " << i << "th read: "
                        "Expected " << i << ", got " << q.front());
            parallel_test_failed = true;
            break;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    BOOST_TEST_MESSAGE(stop_count << " stops during dequeueing");
}

} /* unnamed namespace */

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
    BOOST_CHECK_EQUAL(s.capacity(), 0U);
    BOOST_CHECK_EQUAL(s.size(), 0U);
    s = q;
    BOOST_CHECK(!q.empty());
    BOOST_CHECK(!q.full());
    BOOST_CHECK_EQUAL(q.front(), 5);
    BOOST_CHECK_EQUAL(q.back(), 5);
    BOOST_CHECK(!s.empty());
    BOOST_CHECK(!s.full());
    BOOST_CHECK_EQUAL(s.front(), 5);
    BOOST_CHECK_EQUAL(s.back(), 5);
    BOOST_CHECK_EQUAL(s.capacity(), 4U);
    BOOST_CHECK_EQUAL(s.size(), 1U);
    s = nvwa::fc_queue<int>(5);
    BOOST_CHECK(s.empty());
    BOOST_CHECK(!s.full());
    BOOST_CHECK_EQUAL(s.capacity(), 5U);
    BOOST_CHECK_EQUAL(s.size(), 0U);
    s.push(1);
    BOOST_CHECK_EQUAL(s.front(), 1);
    BOOST_CHECK_EQUAL(s.back(), 1);
    BOOST_CHECK_EQUAL(s.size(), 1U);
    s.push(2);
    BOOST_CHECK_EQUAL(s.front(), 1);
    BOOST_CHECK_EQUAL(s.back(), 2);
    BOOST_CHECK_EQUAL(s.size(), 2U);

    swap(q, s);
    BOOST_CHECK_EQUAL(q.front(), 1);
    BOOST_CHECK_EQUAL(q.back(), 2);
    BOOST_CHECK_EQUAL(q.size(), 2U);
    BOOST_CHECK_EQUAL(s.front(), 5);
    BOOST_CHECK_EQUAL(s.back(), 5);

    check_type<int>();
    check_type<Obj>();
}

BOOST_AUTO_TEST_CASE(fc_queue_parallel_test)
{
    parallel_test_failed = false;
    nvwa::fc_queue<int> q(100'000);
    auto t1 = nvwa::pctimer();
    std::thread enqueue_thread(add_to_queue, std::ref(q));
    std::thread dequeue_thread(read_and_check_queue, std::ref(q));
    enqueue_thread.join();
    dequeue_thread.join();
    BOOST_CHECK(!parallel_test_failed);
    auto t2 = nvwa::pctimer();
    BOOST_TEST_MESSAGE("Test took " << (t2 - t1) << " seconds");
}

BOOST_AUTO_TEST_CASE(fc_queue_parallel_test2)
{
    parallel_test_failed = false;
    nvwa::fc_queue<int> q(100'000);
    auto t1 = nvwa::pctimer();
    std::thread enqueue_thread(add_to_queue2, std::ref(q));
    std::thread dequeue_thread(read_and_check_queue2, std::ref(q));
    enqueue_thread.join();
    dequeue_thread.join();
    BOOST_CHECK(!parallel_test_failed);
    auto t2 = nvwa::pctimer();
    BOOST_TEST_MESSAGE("Test took " << (t2 - t1) << " seconds");
}
