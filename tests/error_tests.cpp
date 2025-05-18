#include <scheduler.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(ErrorTests, convert_error1) {
    TTaskScheduler scheduler;

    auto id = scheduler.add([](int x) { return x; }, 10);

    ASSERT_ANY_THROW(scheduler.getResult<std::string>(id));
}

TEST(ErrorTests, convert_error2) {
    TTaskScheduler scheduler;

    auto id = scheduler.add([](int x) { return x; }, 10);

    ASSERT_ANY_THROW(scheduler.getResult<double>(id));
}

TEST(ErrorTests, no_task_error1) {
    TTaskScheduler scheduler;

    ASSERT_ANY_THROW(scheduler.add([](int x) { return x; }, scheduler.getFutureResult<int>(2)));
}

TEST(ErrorTests, no_task_error2) {
    TTaskScheduler scheduler;

    scheduler.add([](int x) { return x; }, 10);

    ASSERT_ANY_THROW(scheduler.getResult<int>(1));
}
