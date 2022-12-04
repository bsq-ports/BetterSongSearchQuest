#pragma once

#include <chrono>
namespace BetterSongSearch::Util {
    long long CurrentTimeMs(){
        // Set up a timer
        auto lastUpdate = std::chrono::system_clock::now();
        auto milis = std::chrono::duration_cast<std::chrono::milliseconds>(lastUpdate.time_since_epoch());
        auto now = milis.count();
        return now;
    }
}