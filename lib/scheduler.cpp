#include "scheduler.h"

void DependentTask::Execute() {
    if (!scheduler->executed[id].exchange(true)) {
        scheduler->tasks[id]->execute();
        
        std::lock_guard<std::mutex> lock(scheduler->sched_mutex);
        for (int child : scheduler->out_edges[id]) {
            scheduler->in_degree[child]--;
            if (scheduler->in_degree[child] == 0) {
                scheduler->pool.EnqueueTask(std::move(std::make_shared<DependentTask>(child, scheduler)));
            }
        }
    }
}

void TTaskScheduler::executeAll() {
    if (next_id <= 0) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(sched_mutex);
        for (int id = 0; id < next_id; ++id) {
            if (in_degree[id] == 0) {
                pool.EnqueueTask(std::move(std::make_shared<DependentTask>(id, this)));
            }
        }
    }

    pool.WaitIdle();
}
