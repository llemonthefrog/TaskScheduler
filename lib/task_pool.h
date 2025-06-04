#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <atomic>

class BaseTask {
public:
    virtual ~BaseTask() = default;
    virtual void Execute() = 0;
};

class TaskPool final {
public:
    TaskPool(size_t workers_size) : tasks_in_progress(0) {
        for(int i = 0; i < workers_size; i++) {
            workers.emplace_back([this]() { InitWorker(); });
        }
    }

    ~TaskPool() {
        Stop();
    }

    void EnqueueTask(std::shared_ptr<BaseTask> task);
    void WaitIdle();
    void Stop();

private:
    void InitWorker();

    std::vector<std::thread> workers;

    std::queue<std::shared_ptr<BaseTask>> tasks;
    std::mutex queue_mutex;

    std::condition_variable cv_task;
    std::condition_variable cv_idle;

    bool stop = false;
    size_t tasks_in_progress = 0;
};
