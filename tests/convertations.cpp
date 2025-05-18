#include <scheduler.h>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(Convertations, int_to_string1) {
    TTaskScheduler scheduler;

    auto id = scheduler.add([](int x) { return std::to_string(x); }, 10);

    ASSERT_THAT(scheduler.getResult<std::string>(id), "10");
}

TEST(Convertations, int_to_string2) {
    TTaskScheduler scheduler;

    auto id = scheduler.add(
        [](std::string l, std::string r) { 
            return l.append(r); 
        }, 
    "value is: ", "10");

    ASSERT_THAT(scheduler.getResult<std::string>(id), "value is: 10");
}

TEST(Convertations, string_to_int) {
    TTaskScheduler scheduler;

    auto id = scheduler.add([](std::string val) {return std::stoi(val); }, "10");

    ASSERT_THAT(scheduler.getResult<int>(id), 10);
}

TEST(Convertations, vector) {
    TTaskScheduler scheduler;

    auto func = [](const std::vector<int>& vctr) {
        int count = 0;
        for(auto& elem : vctr) {
            count += elem;
        }
        return count;
    };

    std::vector<int> vctr = {1, 2, 3};
    auto id = scheduler.add(func, vctr);

    ASSERT_THAT(scheduler.getResult<int>(id), 6);
}
