#include "core/MinHeapScheduler.h"
#include "core/CronParser.h"
#include <thread>
#include <chrono>
#include <iostream>

/**
 * WHY THIS DESIGN:
 *
 * The scheduler runs in a background thread, checking every second
 * if the top of the min-heap is due. This is the "tick" pattern —
 * common in game engines, schedulers, and event loops.
 *
 * TIME COMPLEXITY:
 * - Check next task: O(1) — just peek at heap top
 * - Execute & reschedule: O(log n) — heap push/pop
 * - Add new task: O(log n) — heap push
 *
 * WHY NOT SLEEP UNTIL NEXT TASK TIME:
 * Because new tasks might be added while sleeping with an earlier
 * run time. Sleeping 1 second is a safe trade-off for simplicity.
 */

// Singleton instance
MinHeapScheduler& MinHeapScheduler::getInstance() {
    static MinHeapScheduler instance;
    return instance;
}

void MinHeapScheduler::addTask(const Task& task) {
    std::lock_guard<std::mutex> lock(heapMutex);
    heap.push(task);
    std::cout << "[Scheduler] Task added: " << task.name
              << " | Next run: " << task.nextRunTime << std::endl;
}

void MinHeapScheduler::removeTask(const std::string& taskId) {
    // WHY REBUILD: std::priority_queue doesn't support random removal.
    // We rebuild the heap without the target task.
    // Trade-off: O(n) removal vs simplicity. Production systems use
    // a custom heap with lazy deletion for O(log n).
    std::lock_guard<std::mutex> lock(heapMutex);
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> newHeap;
    while (!heap.empty()) {
        Task t = heap.top(); heap.pop();
        if (t.id != taskId) newHeap.push(t);
    }
    heap = std::move(newHeap);
}

void MinHeapScheduler::pauseTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(heapMutex);
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> newHeap;
    while (!heap.empty()) {
        Task t = heap.top(); heap.pop();
        if (t.id == taskId) t.status = TaskStatus::PAUSED;
        newHeap.push(t);
    }
    heap = std::move(newHeap);
}

void MinHeapScheduler::resumeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(heapMutex);
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> newHeap;
    while (!heap.empty()) {
        Task t = heap.top(); heap.pop();
        if (t.id == taskId) {
            t.status = TaskStatus::PENDING;
            t.nextRunTime = CronParser::nextRunTime(t.cronExpression);
        }
        newHeap.push(t);
    }
    heap = std::move(newHeap);
}

void MinHeapScheduler::start() {
    running = true;
    std::cout << "[Scheduler] Started. Tasks in queue: " << heap.size() << std::endl;

    // Main scheduler loop — runs in caller's thread
    // Caller should run this in a std::thread
    while (running) {
        std::time_t now = std::time(nullptr);

        {
            std::lock_guard<std::mutex> lock(heapMutex);

            // Process all tasks that are due NOW
            while (!heap.empty()) {
                Task top = heap.top();

                // Not due yet — break, sleep, check again
                if (top.nextRunTime > now) break;

                heap.pop();

                // Skip paused tasks
                if (top.status == TaskStatus::PAUSED) continue;

                // Fire the callback — executor handles actual delivery
                if (onTaskReady) {
                    onTaskReady(top);
                }

                // Reschedule: calculate next run time and re-insert
                try {
                    top.nextRunTime = CronParser::nextRunTime(top.cronExpression, now);
                    top.lastRunTime = now;
                    top.status = TaskStatus::PENDING;
                    heap.push(top);
                    std::cout << "[Scheduler] Rescheduled: " << top.name
                              << " | Next: " << top.nextRunTime << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "[Scheduler] Failed to reschedule " << top.name
                              << ": " << e.what() << std::endl;
                }
            }
        }

        // Sleep 1 second before next tick
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void MinHeapScheduler::stop() {
    running = false;
    std::cout << "[Scheduler] Stopped." << std::endl;
}

int MinHeapScheduler::taskCount() const {
    return heap.size();
}
