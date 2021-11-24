#pragma once

namespace BetterSongSearch {
    int GetMonthsSinceDate(int64_t since)
    {
        auto currentEpoch = std::chrono::system_clock::now().time_since_epoch();
        auto epoch2 = std::chrono::duration<long long>(since);
        auto monthsSince = std::chrono::duration_cast<std::chrono::months>(currentEpoch - epoch2);
        return monthsSince.count();
    }
    std::chrono::time_point<std::chrono::system_clock> GetDateAfterMonths(int sinceEpoch, int addMonths)
    {
        std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> tp{ std::chrono::seconds{sinceEpoch} };
        return tp + std::chrono::months(addMonths);
    }
}