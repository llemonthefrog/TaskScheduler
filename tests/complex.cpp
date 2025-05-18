#include <scheduler.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cctype>
#include <optional>

auto sum = [](const std::vector<int>& vctr) {
    int sum = 0;
    for(auto elem : vctr) {
        sum += elem;
    }

    return sum;
};

TEST(ComplexTest, Parsing) {
    TTaskScheduler scheduler;

    std::vector<std::string> vctr = {"1", "2", "3", "abc", "2d", "20"};

    auto is_alpha = [](const std::string& string) {
        for(char ch : string) {
            if(std::isalpha(ch)) {
                return true;
            }
        }

        return false;
    };

    auto convert = [&is_alpha](const std::vector<std::string>& vctr) {
        std::vector<int> res;
        for(auto& elem : vctr) {
            if(!is_alpha(elem)) {
                res.push_back(std::stoi(elem));
            }
        }

        return res;
    };

    int id1 = scheduler.add(convert, vctr);
    int id2 = scheduler.add(sum, scheduler.getFutureResult<std::vector<int>>(id1));

    ASSERT_THAT(scheduler.getResult<int>(id2), 26);
}

TEST(ComplexTest, DropNullOpt) {
    TTaskScheduler scheduler;

    std::vector<std::optional<int>> vctr = {1, 2, 3, std::nullopt, std::nullopt, std::nullopt, 20};

    auto drop = [](const std::vector<std::optional<int>>& vctr) {
        std::vector<int> res;
        for(auto& elem : vctr) {
            if(elem.has_value()) {
                res.push_back(elem.value());
            }
        }
        return res;
    };

    int id1 = scheduler.add(drop, vctr);
    int id2 = scheduler.add(sum, scheduler.getFutureResult<std::vector<int>>(id1));

    scheduler.executeAll();

    ASSERT_THAT(scheduler.getResult<int>(id2), 26);
}


TEST(ComplexTest, DAG) {
    TTaskScheduler scheduler;

    int a = 10;
    int b = 20;
    int c = 30;

    int id1 = scheduler.add([](int a) { return a; }, a);
    int id2 = scheduler.add([](int b) { return b; }, b);
    int id3 = scheduler.add([](int a, int b) { return a + b; }, 
        scheduler.getFutureResult<int>(id1), scheduler.getFutureResult<int>(id2)); // 30

    int id4 = scheduler.add([](int a, int b) { return a + b; }, 
        c, scheduler.getFutureResult<int>(id3)); // 60

    int id5 = scheduler.add([](int a, int b) { return a + b; },
        scheduler.getFutureResult<int>(id3), scheduler.getFutureResult<int>(id4)); // 90

    ASSERT_THAT(scheduler.getResult<int>(id5), 90);
}
