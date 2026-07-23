#pragma once

#include <string>
#include <vector>
#include <ctime>

/**
 * CronParser converts cron expressions into the next execution time.
 *
 * Cron format:
 * minute hour day month weekday
 *
 * Examples:
 * 0 9 * * 1-5  -> Every weekday at 9:00 AM
 * * * * *      -> Every minute
 * 0 * * * *    -> Every hour
 *
 * Note:
 * Avoid putting the literal five-minute cron expression inside a C-style
 * block comment because its first two characters terminate the comment.
 */

class CronParser
{
public:
    static std::time_t nextRunTime(
        const std::string& cronExpr,
        std::time_t from = 0
    );

    static bool isValid(
        const std::string& cronExpr
    );

private:
    struct CronFields
    {
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
