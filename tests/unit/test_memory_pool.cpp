// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#include <slick/orderbook/detail/memory_pool.hpp>
#include <gtest/gtest.h>
#include <vector>

using namespace slick::orderbook::detail;

// Simple test structure (must be at least sizeof(void*) for ObjectPool)
struct SimpleObject {
    int value;
    int padding;  // Ensure size >= sizeof(void*)
    SimpleObject() : value(0), padding(0) {}
    explicit SimpleObject(int v) : value(v), padding(0) {}
};

// Structure with destructor tracking (must be at least sizeof(void*) for ObjectPool)
struct TrackedObject {
    int value;
    int padding;  // Ensure size >= sizeof(void*)
    static int construct_count;
    static int destruct_count;

    TrackedObject() : value(0), padding(0) { ++construct_count; }
    explicit TrackedObject(int v) : value(v), padding(0) { ++construct_count; }
    ~TrackedObject() { ++destruct_count; }

    static void reset() {
        construct_count = 0;
        destruct_count = 0;
    }
};

int TrackedObject::construct_count = 0;
int TrackedObject::destruct_count = 0;

class ObjectPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        TrackedObject::reset();
    }
};

TEST_F(ObjectPoolTest, InitialState) {
    ObjectPool<SimpleObject> pool(10);
    // Pool uses minimum block size of 64
    EXPECT_GE(pool.capacity(), 10);
    EXPECT_EQ(pool.size(), 0);
    EXPECT_GE(pool.available(), 10);
    EXPECT_TRUE(pool.empty());
}

TEST_F(ObjectPoolTest, AllocateDeallocate) {
    ObjectPool<SimpleObject> pool(10);
    auto initial_capacity = pool.capacity();

    SimpleObject* obj = pool.allocate();
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_EQ(pool.available(), initial_capacity - 1);
    EXPECT_FALSE(pool.empty());

    pool.deallocate(obj);
    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.available(), initial_capacity);
    EXPECT_TRUE(pool.empty());
}

TEST_F(ObjectPoolTest, ConstructDestroy) {
    ObjectPool<TrackedObject> pool(10);

    TrackedObject* obj = pool.construct(42);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 42);
    EXPECT_EQ(TrackedObject::construct_count, 1);
    EXPECT_EQ(pool.size(), 1);

    pool.destroy(obj);
    EXPECT_EQ(TrackedObject::destruct_count, 1);
    EXPECT_EQ(pool.size(), 0);
}

TEST_F(ObjectPoolTest, MultipleAllocations) {
    ObjectPool<SimpleObject> pool(5);
    auto initial_capacity = pool.capacity();

    std::vector<SimpleObject*> objects;
    for (int i = 0; i < 5; ++i) {
        SimpleObject* obj = pool.allocate();
        ASSERT_NE(obj, nullptr);
        objects.push_back(obj);
    }

    EXPECT_EQ(pool.size(), 5);
    EXPECT_EQ(pool.available(), initial_capacity - 5);

    // Deallocate in reverse order
    for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
        pool.deallocate(*it);
    }

    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.available(), initial_capacity);
}

TEST_F(ObjectPoolTest, AutoGrow) {
    ObjectPool<SimpleObject> pool(2);
    auto initial_capacity = pool.capacity();
    EXPECT_GE(initial_capacity, 2);

    // Allocate more than initial capacity
    std::vector<SimpleObject*> objects;
    for (int i = 0; i < 10; ++i) {
        SimpleObject* obj = pool.allocate();
        ASSERT_NE(obj, nullptr);
        objects.push_back(obj);
    }

    EXPECT_GE(pool.capacity(), 10);
    EXPECT_EQ(pool.size(), 10);

    // Cleanup
    for (auto* obj : objects) {
        pool.deallocate(obj);
    }
}

TEST_F(ObjectPoolTest, Reserve) {
    ObjectPool<SimpleObject> pool(5);
    auto initial_capacity = pool.capacity();
    EXPECT_GE(initial_capacity, 5);

    bool success = pool.reserve(20);
    EXPECT_TRUE(success);
    EXPECT_GE(pool.capacity(), 20);
    EXPECT_EQ(pool.size(), 0);
}

TEST_F(ObjectPoolTest, Clear) {
    ObjectPool<SimpleObject> pool(10);
    auto initial_capacity = pool.capacity();

    std::vector<SimpleObject*> objects;
    for (int i = 0; i < 5; ++i) {
        objects.push_back(pool.allocate());
    }

    EXPECT_EQ(pool.size(), 5);
    EXPECT_EQ(pool.available(), initial_capacity - 5);

    pool.clear();
    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.available(), initial_capacity);
}

TEST_F(ObjectPoolTest, ReuseMemory) {
    ObjectPool<SimpleObject> pool(10);

    SimpleObject* obj1 = pool.allocate();
    void* addr1 = obj1;

    pool.deallocate(obj1);

    SimpleObject* obj2 = pool.allocate();
    void* addr2 = obj2;

    // Should reuse the same memory address
    EXPECT_EQ(addr1, addr2);

    pool.deallocate(obj2);
}

TEST_F(ObjectPoolTest, MoveConstructor) {
    ObjectPool<SimpleObject> pool1(10);
    auto initial_capacity = pool1.capacity();
    SimpleObject* obj = pool1.allocate();

    ObjectPool<SimpleObject> pool2(std::move(pool1));

    EXPECT_EQ(pool1.capacity(), 0);
    EXPECT_EQ(pool1.size(), 0);
    EXPECT_EQ(pool2.capacity(), initial_capacity);
    EXPECT_EQ(pool2.size(), 1);

    pool2.deallocate(obj);
}

TEST_F(ObjectPoolTest, MoveAssignment) {
    ObjectPool<SimpleObject> pool1(10);
    auto initial_capacity = pool1.capacity();
    SimpleObject* obj = pool1.allocate();

    ObjectPool<SimpleObject> pool2(5);
    pool2 = std::move(pool1);

    EXPECT_EQ(pool1.capacity(), 0);
    EXPECT_EQ(pool1.size(), 0);
    EXPECT_EQ(pool2.capacity(), initial_capacity);
    EXPECT_EQ(pool2.size(), 1);

    pool2.deallocate(obj);
}

TEST_F(ObjectPoolTest, ConstructWithArguments) {
    ObjectPool<SimpleObject> pool(10);

    SimpleObject* obj1 = pool.construct();
    EXPECT_EQ(obj1->value, 0);

    SimpleObject* obj2 = pool.construct(42);
    EXPECT_EQ(obj2->value, 42);

    pool.deallocate(obj1);
    pool.deallocate(obj2);
}

TEST_F(ObjectPoolTest, StressTest) {
    ObjectPool<SimpleObject> pool(100);

    std::vector<SimpleObject*> allocated;
    allocated.reserve(1000);

    // Allocate 1000 objects
    for (int i = 0; i < 1000; ++i) {
        SimpleObject* obj = pool.allocate();
        ASSERT_NE(obj, nullptr);
        allocated.push_back(obj);
    }

    EXPECT_EQ(pool.size(), 1000);
    EXPECT_GE(pool.capacity(), 1000);

    // Deallocate every other object
    for (size_t i = 0; i < allocated.size(); i += 2) {
        pool.deallocate(allocated[i]);
    }

    EXPECT_EQ(pool.size(), 500);

    // Allocate 500 more (should reuse freed memory)
    for (int i = 0; i < 500; ++i) {
        SimpleObject* obj = pool.allocate();
        ASSERT_NE(obj, nullptr);
    }

    EXPECT_EQ(pool.size(), 1000);

    pool.clear();
    EXPECT_EQ(pool.size(), 0);
}

TEST_F(ObjectPoolTest, LifetimeTracking) {
    TrackedObject::reset();

    {
        ObjectPool<TrackedObject> pool(5);

        TrackedObject* obj1 = pool.construct(1);
        TrackedObject* obj2 = pool.construct(2);
        TrackedObject* obj3 = pool.construct(3);

        EXPECT_EQ(TrackedObject::construct_count, 3);
        EXPECT_EQ(TrackedObject::destruct_count, 0);

        pool.destroy(obj2);
        EXPECT_EQ(TrackedObject::destruct_count, 1);

        pool.destroy(obj1);
        pool.destroy(obj3);
        EXPECT_EQ(TrackedObject::destruct_count, 3);
    }

    // Pool destructor should not call destructors on unallocated objects
    EXPECT_EQ(TrackedObject::construct_count, 3);
    EXPECT_EQ(TrackedObject::destruct_count, 3);
}
