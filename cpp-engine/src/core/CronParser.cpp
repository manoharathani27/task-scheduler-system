#include "core/CronParser.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>

/**
 * WHY THIS IMPLEMENTATION:
 * We walk forward in time from the reference point, second by second
 * (actually minute by minute since cron is minute-granular), checking
 * if the current time matches all 5 cron fields.
 * Simple and correct — production systems use more optimized approaches
 * but this demonstrates the algorithm clearly.
 */

std::vector<int> CronParser::parseField(const std::string& field, int min, int max) {
    std::vector<int> result;

    if (field == "*") {
        // Wildcard — every value in range
        for (int i = min; i <= max; i++) result.push_back(i);
        return result;
    }

    // Handle */n (step values) — e.g. */5 = every 5 units
    if (field.substr(0, 2) == "*/") {
        int step = std::stoi(field.substr(2));
        for (int i = min; i <= max; i += step) result.push_back(i);
        return result;
    }

    // Handle ranges — e.g. 1-5
    if (field.find('-') != std::string::npos) {
        size_t dash = field.find('-');
        int start = std::stoi(field.substr(0, dash));
        int end = std::stoi(field.substr(dash + 1));
        for (int i = start; i <= end; i++) result.push_back(i);
        return result;
    }

    // Handle lists — e.g. 1,3,5
    if (field.find(',') != std::string::npos) {
        std::stringstream ss(field);
        std::string token;
        while (std::getline(ss, token, ',')) {
            result.push_back(std::stoi(token));
        }
        return result;
    }

    // Single value
    result.push_back(std::stoi(field));
    return result;
}

CronParser::CronFields CronParser::parse(const std::string& cronExpr) {
    std::istringstream ss(cronExpr);
    std::vector<std::string> parts;
    std::string part;
    while (ss >> part) parts.push_back(part);

    if (parts.size() != 5) {
        throw std::invalid_argument("Cron expression must have 5 fields: min hour day month weekday");
    }

    CronFields fields;
    fields.minutes  = parseField(parts[0], 0, 59);
    fields.hours    = parseField(parts[1], 0, 23);
    fields.days     = parseField(parts[2], 1, 31);
    fields.months   = parseField(parts[3], 1, 12);
    fields.weekdays = parseField(parts[4], 0, 6);
    return fields;
}

bool CronParser::matches(const CronFields& fields, const std::tm* t) {
    auto contains = [](const std::vector<int>& v, int val) {
        return std::find(v.begin(), v.end(), val) != v.end();
    };

    return contains(fields.minutes,  t->tm_min)
        && contains(fields.hours,    t->tm_hour)
        && contains(fields.days,     t->tm_mday)
        && contains(fields.months,   t->tm_mon + 1)  // tm_mon is 0-based
        && contains(fields.weekdays, t->tm_wday);
}

std::time_t CronParser::nextRunTime(const std::string& cronExpr, std::time_t from) {
    CronFields fields = parse(cronExpr);

    // Start from next minute after 'from'
    if (from == 0) from = std::time(nullptr);
    std::time_t candidate = from + 60;  // start at least 1 minute in future

    // Round down to the minute
    candidate -= (candidate % 60);

    // Walk forward minute by minute (max 1 year = 525960 minutes)
    for (int i = 0; i < 525960; i++) {
        std::tm* t = std::localtime(&candidate);
        if (matches(fields, t)) return candidate;
        candidate += 60;
    }

    throw std::runtime_error("Could not find next run time for: " + cronExpr);
}

bool CronParser::isValid(const std::string& cronExpr) {
    try {
        parse(cronExpr);
        return true;
    } catch (...) {
        return false;
    }
}
