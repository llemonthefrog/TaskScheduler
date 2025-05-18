#include <scheduler.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct Dump {
    int divide(int x) {
        return x / 2;
    }
};

TEST(BasicCases, basic) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x + 10; }, 10);

    ASSERT_THAT(scheduler.getResult<int>(id1), 20);
}

TEST(BasicCases, two_independet_tasks1) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x + 10; }, 10);
    auto id2 = scheduler.add([](int x) { return x + 20; }, 20);

    ASSERT_THAT(scheduler.getResult<int>(id1), 20);
}

TEST(BasicCases, two_independet_tasks2) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x + 10; }, 10);
    auto id2 = scheduler.add([](int x) { return x + 20; }, scheduler.getFutureResult<int>(id1));

    ASSERT_THAT(scheduler.getResult<int>(id1), 20);
}

TEST(BasicCases, dependet_tasks) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x + 10; }, 10);
    auto id2 = scheduler.add([](int x) { return x + 20; }, scheduler.getFutureResult<int>(id1));

    ASSERT_THAT(scheduler.getResult<int>(id2), 40);
}

TEST(BasicCases, all_tasks_execution) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x + 10; }, 10);
    auto id2 = scheduler.add([](int x) { return x + 20; }, scheduler.getFutureResult<int>(id1));

    scheduler.executeAll();

    ASSERT_THAT(scheduler.getResult<int>(id2), 40);
}

TEST(BasicCases, second_type_task) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x, int y) { return x + y + 10; }, 10, 20);

    ASSERT_THAT(scheduler.getResult<int>(id1), 40);
}

TEST(BasicCases, independet_second_type_task) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x; }, 10);
    auto id2 = scheduler.add([](int x, int y) { return x + y + 10; }, 10, 20);

    ASSERT_THAT(scheduler.getResult<int>(id2), 40);
}

TEST(BasicCases, dependet_second_type_task) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x; }, 10);
    auto id2 = scheduler.add([](int y) { return y; }, 20);
    auto id3 = scheduler.add([](int x, int y) { return x + y + 10; }, scheduler.getFutureResult<int>(id1), scheduler.getFutureResult<int>(id2));

    ASSERT_THAT(scheduler.getResult<int>(id3), 40);
}

TEST(BasicCases, execute_all_for_second_type_task) {
    TTaskScheduler scheduler;

    auto id1 = scheduler.add([](int x) { return x; }, 10);
    auto id2 = scheduler.add([](int y) { return y; }, 20);
    auto id3 = scheduler.add([](int x, int y) { return x + y + 10; }, scheduler.getFutureResult<int>(id1), scheduler.getFutureResult<int>(id2));

    scheduler.executeAll();

    ASSERT_THAT(scheduler.getResult<int>(id3), 40);
}

TEST(BasicCases, method) {
    TTaskScheduler scheduler;

    Dump d;
    auto id1 = scheduler.add(&Dump::divide, d, 20);

    ASSERT_THAT(scheduler.getResult<int>(id1), 10);
}
