#pragma once
#include <string>
#include <vector>
#include <ctime>

/**
 * WHY: CronParser converts a cron expression like "*/5 * * * *"
 * into the next execution timestamp.
 *
 * WHAT IS CRON: Cron is a Unix time-based job scheduler.
 * Format: minute hour day month weekday
 * Example: "0 9 * * 1-5" = 9am every weekday
 *
 * WHY SEPARATE CLASS: Single Responsibility Principle (SRP).
 * Parsing logic is isolated here, not mixed into the scheduler.
 * Easy to unit test independently.
 */
class CronParser {
public:
    /**
     * Parse a cron expression and return the next run time
     * after the given reference time.
     *
     * @param cronExpr  Cron string e.g. "* /5 * * * *"
     * @param from      Reference time (defaults to now)
     * @return          Next execution time as Unix timestamp
     */
    static std::time_t nextRunTime(const std::string& cronExpr, std::time_t from = 0);

    /**
     * Validate a cron expression format.
     * Returns true if valid, false otherwise.
     */
    static bool isValid(const std::string& cronExpr);

private:
    struct CronFields {
        std::vector<int> minutes;    // 0-59
        std::vector<int> hours;      // 0-23
        std::vector<int> days;       // 1-31
        std::vector<int> months;     // 1-12
        std::vector<int> weekdays;   // 0-6 (Sun=0)
    };

    static CronFields parse(const std::string& cronExpr);
    static std::vector<int> parseField(const std::string& field, int min, int max);
    static bool matches(const CronFields& fields, const std::tm* t);
};
