// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#include <slick/orderbook/detail/intrusive_list.hpp>
#include <gtest/gtest.h>

using namespace slick::orderbook::detail;

// Test node structure
struct TestNode {
    int value;
    TestNode* next = nullptr;
    TestNode* prev = nullptr;

    explicit TestNode(int v) : value(v) {}
};

class IntrusiveListTest : public ::testing::Test {
protected:
    IntrusiveList<TestNode> list;
};

TEST_F(IntrusiveListTest, InitiallyEmpty) {
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
    EXPECT_EQ(list.front(), nullptr);
    EXPECT_EQ(list.back(), nullptr);
}

TEST_F(IntrusiveListTest, PushBack) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.front(), &node1);
    EXPECT_EQ(list.back(), &node1);

    list.push_back(&node2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front(), &node1);
    EXPECT_EQ(list.back(), &node2);

    list.push_back(&node3);
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.front(), &node1);
    EXPECT_EQ(list.back(), &node3);
}

TEST_F(IntrusiveListTest, PushFront) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_front(&node1);
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.front(), &node1);
    EXPECT_EQ(list.back(), &node1);

    list.push_front(&node2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front(), &node2);
    EXPECT_EQ(list.back(), &node1);

    list.push_front(&node3);
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.front(), &node3);
    EXPECT_EQ(list.back(), &node1);
}

TEST_F(IntrusiveListTest, PopFront) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    list.push_back(&node2);
    list.push_back(&node3);

    TestNode* popped = list.pop_front();
    EXPECT_EQ(popped, &node1);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front(), &node2);

    popped = list.pop_front();
    EXPECT_EQ(popped, &node2);
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.front(), &node3);
    EXPECT_EQ(list.back(), &node3);

    popped = list.pop_front();
    EXPECT_EQ(popped, &node3);
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.front(), nullptr);
    EXPECT_EQ(list.back(), nullptr);
}

TEST_F(IntrusiveListTest, PopBack) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    list.push_back(&node2);
    list.push_back(&node3);

    TestNode* popped = list.pop_back();
    EXPECT_EQ(popped, &node3);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.back(), &node2);

    popped = list.pop_back();
    EXPECT_EQ(popped, &node2);
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.front(), &node1);
    EXPECT_EQ(list.back(), &node1);

    popped = list.pop_back();
    EXPECT_EQ(popped, &node1);
    EXPECT_TRUE(list.empty());
}

TEST_F(IntrusiveListTest, Erase) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    TestNode node4(4);
    TestNode node5(5);

    list.push_back(&node1);
    list.push_back(&node2);
    list.push_back(&node3);
    list.push_back(&node4);
    list.push_back(&node5);

    // Erase middle element
    list.erase(&node3);
    EXPECT_EQ(list.size(), 4);
    EXPECT_EQ(node2.next, &node4);
    EXPECT_EQ(node4.prev, &node2);

    // Erase front
    list.erase(&node1);
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.front(), &node2);
    EXPECT_EQ(node2.prev, nullptr);

    // Erase back
    list.erase(&node5);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.back(), &node4);
    EXPECT_EQ(node4.next, nullptr);

    // Erase remaining
    list.erase(&node2);
    list.erase(&node4);
    EXPECT_TRUE(list.empty());
}

TEST_F(IntrusiveListTest, InsertBefore) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    list.push_back(&node3);

    list.insert_before(&node3, &node2);
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(node1.next, &node2);
    EXPECT_EQ(node2.prev, &node1);
    EXPECT_EQ(node2.next, &node3);
    EXPECT_EQ(node3.prev, &node2);
}

TEST_F(IntrusiveListTest, InsertAfter) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    list.push_back(&node3);

    list.insert_after(&node1, &node2);
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(node1.next, &node2);
    EXPECT_EQ(node2.prev, &node1);
    EXPECT_EQ(node2.next, &node3);
    EXPECT_EQ(node3.prev, &node2);
}

TEST_F(IntrusiveListTest, Iteration) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    list.push_back(&node2);
    list.push_back(&node3);

    // Forward iteration
    int expected = 1;
    for (auto& node : list) {
        EXPECT_EQ(node.value, expected++);
    }
    EXPECT_EQ(expected, 4);

    // Const iteration
    const auto& const_list = list;
    expected = 1;
    for (const auto& node : const_list) {
        EXPECT_EQ(node.value, expected++);
    }
    EXPECT_EQ(expected, 4);
}

TEST_F(IntrusiveListTest, Clear) {
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);

    list.push_back(&node1);
    list.push_back(&node2);
    list.push_back(&node3);

    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
    EXPECT_EQ(list.front(), nullptr);
    EXPECT_EQ(list.back(), nullptr);

    // Verify nodes are unlinked
    EXPECT_EQ(node1.next, nullptr);
    EXPECT_EQ(node1.prev, nullptr);
    EXPECT_EQ(node2.next, nullptr);
    EXPECT_EQ(node2.prev, nullptr);
    EXPECT_EQ(node3.next, nullptr);
    EXPECT_EQ(node3.prev, nullptr);
}

TEST_F(IntrusiveListTest, MoveConstructor) {
    TestNode node1(1);
    TestNode node2(2);

    list.push_back(&node1);
    list.push_back(&node2);

    IntrusiveList<TestNode> list2(std::move(list));
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list2.size(), 2);
    EXPECT_EQ(list2.front(), &node1);
    EXPECT_EQ(list2.back(), &node2);
}

TEST_F(IntrusiveListTest, MoveAssignment) {
    TestNode node1(1);
    TestNode node2(2);

    list.push_back(&node1);
    list.push_back(&node2);

    IntrusiveList<TestNode> list2;
    list2 = std::move(list);

    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list2.size(), 2);
    EXPECT_EQ(list2.front(), &node1);
    EXPECT_EQ(list2.back(), &node2);
}
