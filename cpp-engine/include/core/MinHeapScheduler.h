#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <functional>
#include "core/Task.h"

/**
 * WHY MIN-HEAP (Priority Queue):
 * We need to always know which task runs NEXT (earliest time).
 * A min-heap gives us O(1) access to the minimum element and
 * O(log n) insert/remove. This is much faster than sorting a
 * vector every time (O(n log n)).
 *
 * DSA CONCEPT: Min-Heap / Priority Queue
 * - The task with the smallest nextRunTime is always at the top
 * - When the scheduler "ticks" every second, it just checks the top
 * - If top.nextRunTime <= now → execute it, then re-insert with new time
 *
 * WHY MUTEX: Multiple threads might add/remove tasks simultaneously.
 * std::mutex ensures only one thread touches the heap at a time,
 * preventing data corruption (race condition prevention).
 */
class MinHeapScheduler {
public:
    // Singleton pattern — only one scheduler instance exists
    // WHY SINGLETON: The scheduler is a shared resource. Having two
    // instances would cause duplicate task execution.
    static MinHeapScheduler& getInstance();

    // Prevent copying (Singleton requirement)
    MinHeapScheduler(const MinHeapScheduler&) = delete;
    MinHeapScheduler& operator=(const MinHeapScheduler&) = delete;

    void addTask(const Task& task);
    void removeTask(const std::string& taskId);
    void pauseTask(const std::string& taskId);
    void resumeTask(const std::string& taskId);

    /**
     * Main scheduler loop — runs in its own thread.
     * Every second: check if top task is due → execute → reschedule.
     * WHY SEPARATE THREAD: The scheduler must run continuously without
     * blocking the HTTP API that handles user requests.
     */
    void start();
    void stop();

    int taskCount() const;

    // Callback invoked when a task is ready to execute
    // WHY CALLBACK / std::function: Decouples the scheduler from
    // execution logic. The scheduler only decides WHEN, not HOW.
    std::function<void(const Task&)> onTaskReady;

private:
    MinHeapScheduler() : running(false) {}

    // std::priority_queue with greater<Task> = min-heap
    // WHY: Default priority_queue is max-heap. We override with
    // greater<> to get the smallest nextRunTime at the top.
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> heap;

    std::mutex heapMutex;
    bool running;
};
