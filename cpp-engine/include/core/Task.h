#pragma once
#include <string>
#include <chrono>
#include <ctime>

/**
 * WHY: Task is the core domain entity.
 * It holds all information about a scheduled job —
 * what to run, when to run it, and its current state.
 * Using a struct-like class with clear fields maps
 * directly to the PostgreSQL tasks table.
 */

enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    PAUSED
};

enum class ExecutionStrategy {
    HTTP_CALLBACK,   // POST to a user-defined URL
    PUBSUB           // Publish to GCP Pub/Sub topic
};

struct Task {
    std::string id;               // UUID
    std::string name;
    std::string cronExpression;   // e.g. "*/5 * * * *"
    std::string callbackUrl;      // HTTP endpoint to call
    std::string payload;          // JSON payload to send
    TaskStatus status;
    ExecutionStrategy strategy;
    std::time_t nextRunTime;      // Unix timestamp of next execution
    std::time_t lastRunTime;
    int retryCount;
    int maxRetries;
    std::string executionId;      // Idempotency key per run
    std::string createdBy;        // userId

    // Comparator for min-heap (earlier nextRunTime = higher priority)
    bool operator>(const Task& other) const {
        return nextRunTime > other.nextRunTime;
    }
};
