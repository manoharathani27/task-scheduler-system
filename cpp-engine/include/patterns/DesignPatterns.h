#pragma once
#include "core/Task.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

// ============================================================
// PATTERN 1: STRATEGY PATTERN — Execution Strategy
// ============================================================
/**
 * WHY STRATEGY PATTERN:
 * Different tasks need different execution methods:
 * - Some call an HTTP endpoint (webhook)
 * - Some publish to GCP Pub/Sub
 * Without Strategy, we'd have ugly if/else chains.
 * With Strategy, adding a new execution type (e.g. gRPC)
 * means just adding a new class — no existing code changes.
 * This is the Open/Closed Principle (OCP) from SOLID.
 *
 * REAL WORLD: This is exactly how payment gateways work —
 * Stripe, Razorpay, PayPal all implement the same interface.
 */
class IExecutionStrategy {
public:
    virtual ~IExecutionStrategy() = default;
    virtual bool execute(const Task& task) = 0;  // Pure virtual = interface
    virtual std::string strategyName() const = 0;
};

class HttpCallbackStrategy : public IExecutionStrategy {
public:
    bool execute(const Task& task) override;
    std::string strategyName() const override { return "HTTP_CALLBACK"; }
};

class PubSubStrategy : public IExecutionStrategy {
public:
    bool execute(const Task& task) override;
    std::string strategyName() const override { return "PUBSUB"; }
};


// ============================================================
// PATTERN 2: FACTORY PATTERN — Task Creation
// ============================================================
/**
 * WHY FACTORY PATTERN:
 * Task creation is complex — we need to:
 * 1. Parse the cron expression
 * 2. Calculate first nextRunTime
 * 3. Generate a UUID
 * 4. Set default values
 * 5. Validate fields
 *
 * Callers shouldn't worry about all this. They just say
 * "create me an HTTP task" and the factory handles everything.
 *
 * WHY NOT JUST USE CONSTRUCTORS: Constructors can't return nullptr
 * on failure, can't have descriptive names, and get messy with
 * many optional parameters. Factory solves all three.
 */
class TaskFactory {
public:
    static Task createHttpTask(
        const std::string& name,
        const std::string& cronExpr,
        const std::string& callbackUrl,
        const std::string& payload,
        const std::string& createdBy,
        int maxRetries = 3
    );

    static Task createPubSubTask(
        const std::string& name,
        const std::string& cronExpr,
        const std::string& payload,
        const std::string& createdBy,
        int maxRetries = 3
    );

private:
    static std::string generateUUID();
    static std::string generateExecutionId(const std::string& taskId, std::time_t runTime);
};


// ============================================================
// PATTERN 3: OBSERVER PATTERN — Task Status Notification
// ============================================================
/**
 * WHY OBSERVER PATTERN:
 * When a task completes or fails, multiple things need to happen:
 * - Log to PostgreSQL
 * - Publish status to GCP Pub/Sub
 * - Send webhook callback to user (if configured)
 * - Update in-memory task state
 *
 * Without Observer, TaskExecutor would be tightly coupled to
 * all these systems. With Observer, TaskExecutor just says
 * "task finished" and notifiers handle the rest independently.
 *
 * REAL WORLD: Event-driven architecture used by every major
 * system — Kafka, RabbitMQ, GCP Pub/Sub are all Observer pattern
 * at a distributed scale.
 */
class ITaskObserver {
public:
    virtual ~ITaskObserver() = default;
    virtual void onTaskStarted(const Task& task) = 0;
    virtual void onTaskCompleted(const Task& task) = 0;
    virtual void onTaskFailed(const Task& task, const std::string& reason) = 0;
};

class DatabaseObserver : public ITaskObserver {
public:
    void onTaskStarted(const Task& task) override;
    void onTaskCompleted(const Task& task) override;
    void onTaskFailed(const Task& task, const std::string& reason) override;
};

class LogObserver : public ITaskObserver {
public:
    void onTaskStarted(const Task& task) override;
    void onTaskCompleted(const Task& task) override;
    void onTaskFailed(const Task& task, const std::string& reason) override;
};

// Subject (notifies all observers)
class TaskNotifier {
public:
    void addObserver(std::shared_ptr<ITaskObserver> observer);
    void notifyStarted(const Task& task);
    void notifyCompleted(const Task& task);
    void notifyFailed(const Task& task, const std::string& reason);

private:
    std::vector<std::shared_ptr<ITaskObserver>> observers;
};


// ============================================================
// PATTERN 4: COMMAND PATTERN — Task Execution as Object
// ============================================================
/**
 * WHY COMMAND PATTERN:
 * Encapsulates task execution as an object so we can:
 * - Queue it (execute later)
 * - Log it (what was executed)
 * - Undo it (cancel if not yet run)
 * - Retry it (re-execute the same command)
 *
 * Without Command pattern, execution is a direct function call
 * you can't store, queue, or retry easily.
 *
 * REAL WORLD: Every job queue (BullMQ, Celery, Sidekiq) is
 * essentially the Command pattern — jobs are serialized command
 * objects stored in Redis and executed later.
 */
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual std::string describe() const = 0;
};

class ExecuteTaskCommand : public ICommand {
public:
    ExecuteTaskCommand(const Task& task, std::shared_ptr<IExecutionStrategy> strategy);
    bool execute() override;
    bool undo() override;
    std::string describe() const override;

private:
    Task task;
    std::shared_ptr<IExecutionStrategy> strategy;
    bool executed = false;
};


// ============================================================
// PATTERN 5: REGISTRY PATTERN — Central Task Store
// ============================================================
/**
 * WHY REGISTRY PATTERN:
 * Tasks need to be looked up by ID from multiple places —
 * the scheduler, the API, the worker, the status checker.
 * A central in-memory registry (backed by PostgreSQL) gives
 * O(1) lookups and acts as the single source of truth.
 *
 * WHY unordered_map: Hash map gives O(1) average lookup by taskId.
 * Much faster than a vector O(n) or sorted array O(log n).
 */
class TaskRegistry {
public:
    static TaskRegistry& getInstance();

    void registerTask(const Task& task);
    void unregisterTask(const std::string& taskId);
    Task* findTask(const std::string& taskId);
    void updateTask(const Task& task);
    std::vector<Task> getAllTasks() const;
    int count() const;

private:
    TaskRegistry() = default;
    std::unordered_map<std::string, Task> tasks;
    mutable std::mutex registryMutex;
};
