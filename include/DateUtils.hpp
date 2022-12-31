#pragma once

using namespace std::chrono;

namespace BetterSongSearch {
    int GetMonthsSinceDate(int64_t since)
    {
        auto currentEpoch = std::chrono::system_clock::now().time_since_epoch();
        auto epoch2 = std::chrono::seconds(since);
        auto monthsSince = std::chrono::duration_cast<std::chrono::months>(currentEpoch - epoch2);
        return monthsSince.count();
    }
    // Add calendar months to a date
    seconds GetDateAfterMonths(int64_t sinceEpoch, int64_t addMonths)
    {
        std::chrono::seconds tp(sinceEpoch);

        // Get timepoint
        system_clock::time_point timePoint{tp};
        auto sd = floor<days>(tp);
        // Record the time of day
        auto time_of_day = tp - sd;
        // Convert to a y/m/d calendar data structure
        year_month_day ymd {(sys_days) sd};
        // Add the months
        ymd += months{addMonths};
        // Add some policy for overflowing the day-of-month if desired
        if (!ymd.ok())
            ymd = ymd.year()/ymd.month()/last;
        // Convert back to system_clock::time_point
        system_clock::time_point later = sys_days{ymd} + time_of_day;

        return std::chrono::duration_cast<std::chrono::seconds>(later.time_since_epoch());
    }
}