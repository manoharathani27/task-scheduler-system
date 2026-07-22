#include <iostream>
#include <thread>
#include <memory>
#include <csignal>

#include "core/MinHeapScheduler.h"
#include "core/CronParser.h"
#include "patterns/DesignPatterns.h"

/**
 * WHY MAIN.CPP IS MINIMAL:
 * Main's job is wiring — create components, connect them, start the loop.
 * All real logic lives in dedicated classes. This is the Composition Root
 * pattern — dependencies are assembled here and injected into components.
 */

bool running = true;

void signalHandler(int signal) {
    std::cout << "\n[Engine] Shutdown signal received. Stopping..." << std::endl;
    running = false;
    MinHeapScheduler::getInstance().stop();
}

int main() {
    std::cout << "=== Distributed Task Scheduler — C++ Engine ===" << std::endl;

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Setup observers (Observer Pattern)
    TaskNotifier notifier;
    notifier.addObserver(std::make_shared<LogObserver>());
    notifier.addObserver(std::make_shared<DatabaseObserver>());

    // Setup execution strategies (Strategy Pattern)
    auto httpStrategy  = std::make_shared<HttpCallbackStrategy>();
    auto pubsubStrategy = std::make_shared<PubSubStrategy>();

    // Wire scheduler callback
    // WHY LAMBDA: Captures the notifier and strategies without global state.
    // When a task is due, the scheduler calls this function.
    MinHeapScheduler::getInstance().onTaskReady = [&](const Task& task) {
        notifier.notifyStarted(task);

        // Select strategy based on task type (Strategy Pattern in action)
        std::shared_ptr<IExecutionStrategy> strategy =
            (task.strategy == ExecutionStrategy::HTTP_CALLBACK)
                ? httpStrategy
                : pubsubStrategy;

        // Wrap in Command (Command Pattern)
        ExecuteTaskCommand cmd(task, strategy);
        bool success = cmd.execute();

        if (success) {
            notifier.notifyCompleted(task);
        } else {
            notifier.notifyFailed(task, "Execution returned false");
        }
    };

    // Seed with demo tasks (Factory Pattern creates properly initialized tasks)
    try {
        Task t1 = TaskFactory::createHttpTask(
            "Health Check",
            "*/1 * * * *",           // Every minute
            "http://api:3000/health",
            "{}",
            "system"
        );

        Task t2 = TaskFactory::createPubSubTask(
            "Daily Report",
            "0 9 * * *",             // Every day at 9am
            "{\"type\": \"daily_report\"}",
            "system"
        );

        // Register in registry (Registry Pattern)
        TaskRegistry::getInstance().registerTask(t1);
        TaskRegistry::getInstance().registerTask(t2);

        // Add to scheduler
        MinHeapScheduler::getInstance().addTask(t1);
        MinHeapScheduler::getInstance().addTask(t2);

        std::cout << "[Engine] " << TaskRegistry::getInstance().count()
                  << " tasks loaded. Starting scheduler..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Engine] Failed to create tasks: " << e.what() << std::endl;
        return 1;
    }

    // Start the scheduler loop in a background thread
    // WHY THREAD: Scheduler loop is blocking. We need the main thread
    // free for signal handling and future HTTP server integration.
    std::thread schedulerThread([]() {
        MinHeapScheduler::getInstance().start();
    });

    std::cout << "[Engine] Scheduler running. Press Ctrl+C to stop." << std::endl;

    // Wait for shutdown
    schedulerThread.join();

    std::cout << "[Engine] Shutdown complete." << std::endl;
    return 0;
}
