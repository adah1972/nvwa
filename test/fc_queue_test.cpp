#include "nvwa/fc_queue.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <boost/core/demangle.hpp>
#include <boost/test/unit_test.hpp>
#include "nvwa/pctimer.h"

#if __has_include(<memory_resource>)
#include <memory_resource>
using test_alloc = std::pmr::polymorphic_allocator<int>;
#else
using test_alloc = std::allocator<int>;
#endif

using namespace boost::unit_test_framework;

namespace {

class Obj {
public:
    explicit Obj(int n) : value_(n) {}
    int value() const
    {
        return value_;
    }

private:
    int value_;
};

const int LOOPS = 10'000'000;
std::atomic<bool> parallel_test_failed{false};
std::mutex output_mtx;

void pause()
{
    std::this_thread::sleep_for(std::chrono::microseconds(100));
}

void add_to_queue(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        while (q.full()) {
            pause();
            if (parallel_test_failed) {
                return;
            }
            ++stop_count;
        }
        q.push(i);
    }
    std::lock_guard guard{output_mtx};
    BOOST_TEST_MESSAGE(stop_count << " stops during enqueueing");
}

void read_and_check_queue(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        while (q.empty()) {
            pause();
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
    std::lock_guard guard{output_mtx};
    BOOST_TEST_MESSAGE(stop_count << " stops during dequeueing");
}

void add_to_queue2(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        while (!q.write(i)) {
            pause();
            if (parallel_test_failed) {
                return;
            }
            ++stop_count;
        }
    }
    std::lock_guard guard{output_mtx};
    BOOST_TEST_MESSAGE(stop_count << " stops during enqueueing");
}

void read_and_check_queue2(nvwa::fc_queue<int>& q)
{
    int stop_count = 0;
    for (int i = 0; i < LOOPS; ++i) {
        int value{};
        while (!q.read(value)) {
            pause();
            ++stop_count;
        }
        if (i != value) {
            BOOST_ERROR("Failure on " << i << "th read: "
                        "Expected " << i << ", got " << q.front());
            parallel_test_failed = true;
            break;
        }
    }
    std::lock_guard guard{output_mtx};
    BOOST_TEST_MESSAGE(stop_count << " stops during dequeueing");
}

} /* unnamed namespace */

BOOST_AUTO_TEST_CASE(fc_queue_basic_test)
{
    nvwa::fc_queue<int, test_alloc> q(4);
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

    nvwa::fc_queue<int, test_alloc> r(q);
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

    nvwa::fc_queue<int, test_alloc> s;
    BOOST_CHECK(s.empty());
    BOOST_CHECK(s.full());
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
    s = nvwa::fc_queue<int, test_alloc>(5);
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
}

BOOST_AUTO_TEST_CASE(fc_queue_emplace_test)
{
    nvwa::fc_queue<Obj> q(4);
    q.emplace(1);
    q.emplace(2);
    BOOST_CHECK(q.size() == 2U);
    BOOST_CHECK(q.back().value() == 2);
}

#if __has_include(<memory_resource>)
BOOST_AUTO_TEST_CASE(fc_queue_polymorphic_allocator_test)
{
    nvwa::fc_queue<int, std::pmr::polymorphic_allocator<int>> t(4);
    BOOST_CHECK_EQUAL(t.capacity(), 4U);
    BOOST_CHECK_EQUAL(t.size(), 0U);
    BOOST_CHECK(!t.full());
    BOOST_CHECK(t.empty());
    t.push(1);
    BOOST_CHECK_EQUAL(t.size(), 1U);

    std::pmr::unsynchronized_pool_resource res;
    std::pmr::polymorphic_allocator<int> a{&res};
    nvwa::fc_queue<int, std::pmr::polymorphic_allocator<int>> u{a};
    u = t;
    BOOST_CHECK_EQUAL(u.capacity(), 4U);
    BOOST_CHECK_EQUAL(u.size(), 1U);
}
#endif

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
