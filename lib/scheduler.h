#pragma once

#include <unordered_map>
#include <queue>
#include <vector>
#include <cinttypes>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <memory>

#include "any_type.h"
#include "task_pool.h"

class TTaskScheduler;

class BaseSchedule {
public:
    virtual void execute() = 0;
    virtual AnyType result() = 0;
    virtual bool has_value() = 0;
    virtual ~BaseSchedule() = default;
};

class DependentTask : public BaseTask {
    int id;
    TTaskScheduler* scheduler;

public:
    DependentTask(int id_, TTaskScheduler* sch) : id(id_), scheduler(sch) {}
    void Execute();
};
    
template<typename T>
struct Promise {
    explicit Promise(T val) noexcept : value(val), vertex(nullptr), promised(false), id(-1) {}

    explicit Promise(BaseSchedule* vert, int id) noexcept : vertex(vert), id(id) {
        promised = true;
    }

    using value_type = T;

    BaseSchedule* vertex;
    T value;
    bool promised;
    int id;
};

template<typename Functor, typename T>
class ScheduleOfOne : public BaseSchedule {
    Functor func_;
    Promise<T> arg_;
    AnyType result_;
public:
    ScheduleOfOne(Functor func, const Promise<T>& arg) noexcept : 
        arg_(arg), func_(func) {}

    void execute() override {
        using type = typename Promise<T>::value_type;

        if(arg_.promised) {
            if (!arg_.vertex->has_value()) {
                throw std::runtime_error("Promised vertex has no value");
            }

            type data = any_cast<type>(arg_.vertex->result());
            result_ = func_(data);
        } else {
            result_ = func_(arg_.value);
        }
    }

    AnyType result() override {
        return result_.clone();
    }

    bool has_value() override {
        return result_.has_value();
    }
};

template<typename Functor, typename T, typename U = T>
class ScheduleOfTwo : public BaseSchedule {
    Functor func_;
    Promise<T> arg_left_;
    Promise<U> arg_right_;
    AnyType result_;
public:
    ScheduleOfTwo(Functor func, const Promise<T>& arg_left, const Promise<U>& arg_right) noexcept :
        func_(func), arg_left_(arg_left), arg_right_(arg_right) {}

    void execute() override {
        T left_arg;
        U right_arg;

        if(arg_left_.promised) {
            if(!arg_left_.vertex->has_value()) {
                throw std::runtime_error("not provided value for left arg");
            }

            left_arg = any_cast<T>(arg_left_.vertex->result());
        } else {
            left_arg = arg_left_.value;
        }

        if(arg_right_.promised) {
            if(!arg_right_.vertex->has_value()) {
                throw std::runtime_error("not provided value for left arg");
            }

            right_arg = any_cast<U>(arg_right_.vertex->result());
        } else {
            right_arg = arg_right_.value;
        }

        result_ = func_(left_arg, right_arg);
    }

    AnyType result() override {
        return result_.clone();
    }

    bool has_value() override {
        return result_.has_value();
    }
};

template<typename Class, typename RetType, typename Arg>
class ScheduleOfOneMethod : public BaseSchedule {
    RetType (Class::*func_)(Arg);
    Promise<Arg> arg_;
    Class obj_;
    AnyType result_;
public:
    ScheduleOfOneMethod(RetType (Class::*func)(Arg), Class obj, const Promise<Arg>& arg) noexcept :
        func_(func), obj_(obj), arg_(arg) {}

    void execute() override {
        using type = typename Promise<Arg>::value_type;

        if(arg_.promised) {
            if (!arg_.vertex->has_value()) {
                throw std::runtime_error("Promised vertex has no value");
            }

            type data = any_cast<type>(arg_.vertex->result());
            result_ = (obj_.*func_)(data);
        } else {
            result_ = (obj_.*func_)(arg_.value);
        }
    }

    AnyType result() override {
        return result_.clone();
    }

    bool has_value() override {
        return result_.has_value();
    }
};

class TTaskScheduler {
public:
    std::unordered_map<int, std::shared_ptr<BaseSchedule>> tasks;
    std::unordered_map<int, std::atomic<int>> in_degree;
    std::unordered_map<int, std::vector<int>> out_edges;
    std::unordered_map<int, std::atomic<bool>> executed;

    std::mutex sched_mutex;

    TaskPool pool;

    int next_id = 0;

public:
    TTaskScheduler() : pool(4) {}
    TTaskScheduler(size_t workes_count) : pool(workes_count) {}

    template<typename Functor, typename T>
    int add(Functor func, T val) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfOne<Functor,T>>(ScheduleOfOne<Functor, T>(func, Promise<T>(val)));
        in_degree[next_id] = 0;

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T, typename U = T>
    int add(Functor func, T val_left, U val_right) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfTwo<Functor,T,U>>(ScheduleOfTwo<Functor, T, U>(func, Promise<T>(val_left), Promise<U>(val_right)));
        in_degree[next_id] = 0;

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T>
    int add(Functor func, Promise<T> promise) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfOne<Functor, T>>(ScheduleOfOne<Functor, T>(func, promise));

        out_edges[promise.id].push_back(next_id);
        in_degree[next_id] += 1;

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T, typename U = T>
    int add(Functor func, T val, Promise<U> promise) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfTwo<Functor,T,U>>(ScheduleOfTwo<Functor, T, U>(func, Promise<T>(val), promise));

        out_edges[promise.id].push_back(next_id);
        in_degree[next_id] += 1;

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T, typename U = T>
    int add(Functor func, Promise<T> promise_right, Promise<U> promise_left) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfTwo<Functor, T, U>>(ScheduleOfTwo<Functor, T, U>(func, promise_right, promise_left));
        
        out_edges[promise_left.id].push_back(next_id);
        out_edges[promise_right.id].push_back(next_id);
        in_degree[next_id] += 2;

        next_id++;
        return next_id - 1;
    }

    template<typename Class, typename RetType, typename Arg>
    int add(RetType (Class::*func)(Arg), Class obj, Arg val) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfOneMethod<Class, RetType, Arg>>(
                ScheduleOfOneMethod<Class, RetType, Arg>(func, obj, Promise<Arg>(val)));
        in_degree[next_id] = 0;

        next_id++;
        return next_id - 1;
    }

    template<typename Class, typename RetType, typename Arg>
    int add(RetType (Class::*func)(Arg), Class obj, Promise<Arg> promise) {
        tasks[next_id] = 
            std::make_shared<ScheduleOfOneMethod<Class, RetType, Arg>>(
                ScheduleOfOneMethod<Class, RetType, Arg>(func, obj, promise));

        out_edges[promise.id].push_back(next_id);
        in_degree[next_id] += 1;

        next_id++;
        return next_id - 1;
    }

    template<typename T>
    Promise<T> getFutureResult(int id) {
        if(tasks.find(id) == tasks.end()) {
            throw std::runtime_error("there is no task with such id");
        }

        return Promise<T>(tasks[id].get(), id);
    }

    template<typename T>
    auto getResult(int id) {
        if (tasks.find(id) == tasks.end()) {
            throw std::runtime_error("Invalid task id");
        }

        {
            std::lock_guard<std::mutex> lock(sched_mutex);
            for (int i = 0; i < next_id; ++i) {
                if (in_degree[i] == 0) {
                    pool.EnqueueTask(std::make_shared<DependentTask>(i, this));
                }
            }
        }

        pool.WaitIdle();
        return any_cast<T>(tasks[id]->result());
    }

    void executeAll();
};
