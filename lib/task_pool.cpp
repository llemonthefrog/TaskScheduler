#include "task_pool.h"

void TaskPool::EnqueueTask(std::shared_ptr<BaseTask> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(task);
    }
    cv_task.notify_one();
}

void TaskPool::WaitIdle() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    cv_idle.wait(lock, [this]() {
        return tasks.empty() && tasks_in_progress == 0;
    });
}

void TaskPool::Stop() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    cv_task.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void TaskPool::InitWorker() {
    while(true) {
        std::shared_ptr<BaseTask> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv_task.wait(lock, [this]() {
                return stop || !tasks.empty();
            });

            if(stop && tasks.empty()) {
                return;
            }

            task = tasks.front();
            tasks.pop();
            tasks_in_progress++;
        }

        task->Execute();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks_in_progress--;
            if (tasks.empty() && tasks_in_progress == 0) {
                cv_idle.notify_all();
            }
        }
    }
}
