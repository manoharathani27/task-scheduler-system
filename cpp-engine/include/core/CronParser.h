#pragma once

#include <string>
#include <vector>
#include <ctime>

/**
 * WHY:
 * CronParser converts cron expressions into the next execution timestamp.
 *
 * WHAT IS CRON:
 * Cron is a Unix time-based job scheduler.
 *
 * Format:
 * minute hour day month weekday
 *
 * Examples:
 * "0 9 * * 1-5"  -> 9:00 AM every weekday
 * "* * * * *"    -> every minute
 * "0 * * * *"    -> every hour
 *
 * NOTE:
 * Do NOT write the expression "*/5 * * * *" inside a block comment because
 * the characters "*/" terminate the comment in C++.
 *
 * WHY SEPARATE CLASS:
 * Follows the Single Responsibility Principle (SRP).
 * All cron parsing logic stays here instead of inside the scheduler,
 * making it easier to test and maintain.
 */
class CronParser {
public:
    /**
     * Parse a cron expression and return the next run time
     * after the given reference time.
     *
     * @param cronExpr Cron expression.
     * @param from Reference time (defaults to now).
     * @return Next execution time as Unix timestamp.
     */
    static std::time_t nextRunTime(const std::string& cronExpr,
                                   std::time_t from = 0);

    /**
     * Validate a cron expression.
     *
     * @param cronExpr Cron expression.
     * @return true if valid.
     */
    static bool isValid(const std::string& cronExpr);

private:
    struct CronFields {
        std::vector<int> minutes;
        std::vector<int> hours;
        std::vector<int> days;
        std::vector<int> months;
        std::vector<int> weekdays;
    };

    static CronFields parse(const std::string& cronExpr);

    static std::vector<int> parseField(
        const std::string& field,
        int min,
        int max
    );

    static bool matches(
        const CronFields& fields,
        const std::tm* t
    );
};