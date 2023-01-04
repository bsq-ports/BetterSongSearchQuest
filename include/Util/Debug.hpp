#pragma once

#include <string>
#include "main.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"
using namespace std;


namespace BetterSongSearch::Util
{
    string SortToString(int sortMode);

    string RequirementTypeToString(int requirementType);

    string CharFilterTypeToString(int charFilterType);

    string DifficultyFilterTypeToString(int diffFilterType);

    string RankedFilterTypeToString(int rankedFilterType);
    string LocalScoreFilterTypeToString(int localScoreFilterType);

    string DownloadFilterTypeToString(int downloadFilterType);

    // Logs song info for checking sort and options
    void LogSongInfo(const SDC_wrapper::BeatStarSong* song);

    // Prints to the provided buffer a nice number of bytes (KB, MB, GB, etc)
    string pretty_bytes(int64_t bytes);
}
