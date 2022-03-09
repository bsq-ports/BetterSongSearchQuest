#pragma once

namespace BetterSongSearch {
    int GetMonthsSinceDate(int64_t since)
    {
        auto currentEpoch = std::chrono::system_clock::now().time_since_epoch();
        auto epoch2 = std::chrono::seconds(since);
        auto monthsSince = std::chrono::duration_cast<std::chrono::months>(currentEpoch - epoch2);
        return monthsSince.count();
    }
    auto GetDateAfterMonths(int64_t sinceEpoch, int64_t addMonths)
    {
        std::chrono::seconds tp(sinceEpoch);
        return tp + std::chrono::months(addMonths);
    }
}