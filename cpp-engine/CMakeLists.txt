cmake_minimum_required(VERSION 3.16)
project(TaskSchedulerEngine VERSION 1.0.0 LANGUAGES CXX)

# WHY C++17: std::optional, structured bindings, and filesystem
# support needed for clean modern C++ code
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/core/CronParser.cpp
    src/core/MinHeapScheduler.cpp
    src/patterns/DesignPatterns.cpp
)

# Build executable
add_executable(task-scheduler-engine ${SOURCES})

# Link threads — needed for std::thread and std::mutex
# WHY: std::thread requires pthreads on Linux
find_package(Threads REQUIRED)
target_link_libraries(task-scheduler-engine Threads::Threads)

# Compiler warnings — good practice
target_compile_options(task-scheduler-engine PRIVATE -Wall -Wextra -O2)
