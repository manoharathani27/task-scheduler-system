#include "patterns/DesignPatterns.h"
#include "core/CronParser.h"
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <chrono>
#include <mutex>

// ============================================================
// STRATEGY PATTERN IMPLEMENTATIONS
// ============================================================

/**
 * WHY HTTP CALLBACK:
 * The most flexible execution type. The task scheduler sends a
 * POST request to the user's endpoint. The user's service handles
 * the actual work. This is how Stripe webhooks, GitHub Actions
 * triggers, and most SaaS schedulers work.
 */
bool HttpCallbackStrategy::execute(const Task& task) {
    // In production: use libcurl or cpp-httplib for real HTTP calls
    // Here we simulate and log the call
    std::cout << "[HTTP Strategy] POST " << task.callbackUrl
              << " | Payload: " << task.payload
              << " | ExecutionId: " << task.executionId << std::endl;

    // Idempotency: ExecutionId ensures if this fires twice (network retry),
    // the receiver can detect and ignore the duplicate
    return true;
}

/**
 * WHY PUBSUB STRATEGY:
 * For high-volume tasks, publishing to GCP Pub/Sub decouples
 * the scheduler from execution. Workers scale independently.
 * If execution is slow, messages queue up — no data loss.
 */
bool PubSubStrategy::execute(const Task& task) {
    std::cout << "[PubSub Strategy] Publishing task: " << task.id
              << " | Topic: task-scheduler-topic"
              << " | ExecutionId: " << task.executionId << std::endl;
    // In production: use GCP Pub/Sub C++ client library
    return true;
}

// ============================================================
// FACTORY PATTERN IMPLEMENTATIONS
// ============================================================

std::string TaskFactory::generateUUID() {
    // Simple UUID v4 generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

std::string TaskFactory::generateExecutionId(const std::string& taskId, std::time_t runTime) {
    // WHY EXECUTION ID: Idempotency key — even if Pub/Sub delivers
    // the message twice, the worker checks this ID and skips duplicates.
    return taskId + "_" + std::to_string(runTime);
}

Task TaskFactory::createHttpTask(
    const std::string& name,
    const std::string& cronExpr,
    const std::string& callbackUrl,
    const std::string& payload,
    const std::string& createdBy,
    int maxRetries
) {
    if (!CronParser::isValid(cronExpr)) {
        throw std::invalid_argument("Invalid cron expression: " + cronExpr);
    }

    Task task;
    task.id = generateUUID();
    task.name = name;
    task.cronExpression = cronExpr;
    task.callbackUrl = callbackUrl;
    task.payload = payload;
    task.createdBy = createdBy;
    task.strategy = ExecutionStrategy::HTTP_CALLBACK;
    task.status = TaskStatus::PENDING;
    task.maxRetries = maxRetries;
    task.retryCount = 0;
    task.nextRunTime = CronParser::nextRunTime(cronExpr);
    task.executionId = generateExecutionId(task.id, task.nextRunTime);
    return task;
}

Task TaskFactory::createPubSubTask(
    const std::string& name,
    const std::string& cronExpr,
    const std::string& payload,
    const std::string& createdBy,
    int maxRetries
) {
    if (!CronParser::isValid(cronExpr)) {
        throw std::invalid_argument("Invalid cron expression: " + cronExpr);
    }

    Task task;
    task.id = generateUUID();
    task.name = name;
    task.cronExpression = cronExpr;
    task.payload = payload;
    task.createdBy = createdBy;
    task.strategy = ExecutionStrategy::PUBSUB;
    task.status = TaskStatus::PENDING;
    task.maxRetries = maxRetries;
    task.retryCount = 0;
    task.nextRunTime = CronParser::nextRunTime(cronExpr);
    task.executionId = generateExecutionId(task.id, task.nextRunTime);
    return task;
}

// ============================================================
// OBSERVER PATTERN IMPLEMENTATIONS
// ============================================================

void DatabaseObserver::onTaskStarted(const Task& task) {
    std::cout << "[DB Observer] UPDATE tasks SET status='RUNNING' WHERE id='" << task.id << "'" << std::endl;
    // In production: Sequelize/libpq SQL call
}

void DatabaseObserver::onTaskCompleted(const Task& task) {
    std::cout << "[DB Observer] UPDATE tasks SET status='COMPLETED', last_run=NOW() WHERE id='" << task.id << "'" << std::endl;
}

void DatabaseObserver::onTaskFailed(const Task& task, const std::string& reason) {
    std::cout << "[DB Observer] UPDATE tasks SET status='FAILED', failure_reason='" << reason
              << "' WHERE id='" << task.id << "'" << std::endl;
}

void LogObserver::onTaskStarted(const Task& task) {
    std::cout << "[Log Observer] STARTED: " << task.name << " | ID: " << task.id << std::endl;
}

void LogObserver::onTaskCompleted(const Task& task) {
    std::cout << "[Log Observer] COMPLETED: " << task.name << " | ExecutionId: " << task.executionId << std::endl;
}

void LogObserver::onTaskFailed(const Task& task, const std::string& reason) {
    std::cerr << "[Log Observer] FAILED: " << task.name << " | Reason: " << reason << std::endl;
}

void TaskNotifier::addObserver(std::shared_ptr<ITaskObserver> observer) {
    observers.push_back(observer);
}

void TaskNotifier::notifyStarted(const Task& task) {
    for (auto& obs : observers) obs->onTaskStarted(task);
}

void TaskNotifier::notifyCompleted(const Task& task) {
    for (auto& obs : observers) obs->onTaskCompleted(task);
}

void TaskNotifier::notifyFailed(const Task& task, const std::string& reason) {
    for (auto& obs : observers) obs->onTaskFailed(task, reason);
}

// ============================================================
// COMMAND PATTERN IMPLEMENTATIONS
// ============================================================

ExecuteTaskCommand::ExecuteTaskCommand(const Task& task, std::shared_ptr<IExecutionStrategy> strategy)
    : task(task), strategy(strategy) {}

bool ExecuteTaskCommand::execute() {
    std::cout << "[Command] Executing: " << describe() << std::endl;
    executed = strategy->execute(task);
    return executed;
}

bool ExecuteTaskCommand::undo() {
    // For HTTP callbacks: send a cancel request to the callbackUrl
    // For Pub/Sub: publish a CANCEL message
    // In this implementation, we log the undo attempt
    if (executed) {
        std::cout << "[Command] Undo: " << task.name << " (cancel signal sent)" << std::endl;
        executed = false;
    }
    return true;
}

std::string ExecuteTaskCommand::describe() const {
    return "Task[" + task.id + "] '" + task.name + "' via " + strategy->strategyName();
}

// ============================================================
// REGISTRY PATTERN IMPLEMENTATIONS
// ============================================================

TaskRegistry& TaskRegistry::getInstance() {
    static TaskRegistry instance;
    return instance;
}

void TaskRegistry::registerTask(const Task& task) {
    std::lock_guard<std::mutex> lock(registryMutex);
    tasks[task.id] = task;
}

void TaskRegistry::unregisterTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(registryMutex);
    tasks.erase(taskId);
}

Task* TaskRegistry::findTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(registryMutex);
    auto it = tasks.find(taskId);
    return it != tasks.end() ? &it->second : nullptr;
}

void TaskRegistry::updateTask(const Task& task) {
    std::lock_guard<std::mutex> lock(registryMutex);
    tasks[task.id] = task;
}

std::vector<Task> TaskRegistry::getAllTasks() const {
    std::lock_guard<std::mutex> lock(registryMutex);
    std::vector<Task> result;
    for (const auto& pair : tasks) result.push_back(pair.second);
    return result;
}

int TaskRegistry::count() const {
    std::lock_guard<std::mutex> lock(registryMutex);
    return tasks.size();
}
