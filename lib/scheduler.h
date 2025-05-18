#pragma once

#include <unordered_map>
#include <vector>
#include <stack>

#include "any_type.h"

class BaseSchedule {
public:
    virtual void execute() = 0;
    virtual AnyType result() = 0;
    virtual bool has_value() = 0;
    virtual ~BaseSchedule() = default;
};
    
template<typename T>
struct Promise {
    explicit Promise(T val) noexcept : value(val), vertex(nullptr) {
        promised = false;
    }

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
                throw "Promised vertex has no value";
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
                throw "not provided value for left arg";
            }

            left_arg = any_cast<T>(arg_left_.vertex->result());
        } else {
            left_arg = arg_left_.value;
        }

        if(arg_right_.promised) {
            if(!arg_right_.vertex->has_value()) {
                throw "not provided value for left arg";
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
                throw "Promised vertex has no value";
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
    std::unordered_map<int, std::unique_ptr<BaseSchedule>> tasks;
    std::unordered_map<int, std::vector<int>> tasks_graph;
    int next_id = 0;
    
private:
    void visit(std::stack<int>& stack, std::vector<bool>& visited, int vertex) {
        visited[vertex] = true;
        
        for(auto& u : tasks_graph[vertex]) {
            if(!visited[u]) {
                visit(stack, visited, u);
            }
        }

        stack.push(vertex);
    }

    std::stack<int> top_sort() {
        std::vector<bool> visited(next_id, false);
        std::stack<int> stack;

        for(int i = 0; i < next_id; i++) {
            if(!visited[i]) {
                visit(stack, visited, i);
            }
        }
        
        return stack;
    }

public:
    template<typename Functor, typename T>
    int add(Functor func, T val) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfOne<Functor,T>>(ScheduleOfOne<Functor, T>(func, Promise<T>(val)));
        tasks_graph[next_id] = {};

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T, typename U = T>
    int add(Functor func, T val_left, U val_right) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfTwo<Functor,T,U>>(ScheduleOfTwo<Functor, T, U>(func, Promise<T>(val_left), Promise<U>(val_right)));
        tasks_graph[next_id] = {};

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T>
    int add(Functor func, Promise<T> promise) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfOne<Functor, T>>(ScheduleOfOne<Functor, T>(func, promise));
        tasks_graph[next_id] = {};

        tasks_graph[promise.id].push_back(next_id);

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T, typename U = T>
    int add(Functor func, T val, Promise<U> promise) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfTwo<Functor,T,U>>(ScheduleOfTwo<Functor, T, U>(func, Promise<T>(val), promise));
        tasks_graph[next_id] = {};

        tasks_graph[promise.id].push_back(next_id);

        next_id++;
        return next_id - 1;
    }

    template<typename Functor, typename T, typename U = T>
    int add(Functor func, Promise<T> promise_right, Promise<U> promise_left) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfTwo<Functor, T, U>>(ScheduleOfTwo<Functor, T, U>(func, promise_right, promise_left));

        tasks_graph[next_id] = {};
        
        tasks_graph[promise_left.id].push_back(next_id);
        tasks_graph[promise_right.id].push_back(next_id);

        next_id++;
        return next_id - 1;
    }

    template<typename Class, typename RetType, typename Arg>
    int add(RetType (Class::*func)(Arg), Class obj, Arg val) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfOneMethod<Class, RetType, Arg>>(
                ScheduleOfOneMethod<Class, RetType, Arg>(func, obj, Promise<Arg>(val)));
        tasks_graph[next_id] = {};

        next_id++;
        return next_id - 1;
    }

    template<typename Class, typename RetType, typename Arg>
    int add(RetType (Class::*func)(Arg), Class obj, Promise<Arg> promise) {
        tasks[next_id] = 
            std::make_unique<ScheduleOfOneMethod<Class, RetType, Arg>>(
                ScheduleOfOneMethod<Class, RetType, Arg>(func, obj, promise));
        tasks_graph[next_id] = {};

        tasks_graph[promise.id].push_back(next_id);

        next_id++;
        return next_id - 1;
    }

    template<typename T>
    Promise<T> getFutureResult(int id) {
        if(tasks.find(id) == tasks.end()) {
            throw "there is no task with such id";
        }

        return Promise<T>(tasks[id].get(), id);
    }

    template<typename T>
    auto getResult(int id) {
        if(tasks.find(id) == tasks.end()) {
            throw "there is no task with such id";
        }

        if(tasks[id].get()->has_value()) {
            return any_cast<T>(tasks[id].get()->result());
        }

        auto stack = top_sort();

        while(!stack.empty()) {
            int next = stack.top();
            tasks[next].get()->execute();

            if(next == id) {
                break;
            }

            stack.pop();
            next = stack.top();
        }

        return any_cast<T>(tasks[id].get()->result());
    }

    void executeAll() {
        auto stack = top_sort();

        while(!stack.empty()) {
            int next = stack.top();
            tasks[next].get()->execute();

            stack.pop();
        }
    }
};
