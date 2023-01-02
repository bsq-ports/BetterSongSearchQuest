#pragma once

#include <string>
#include "main.hpp"
using namespace std;


namespace BetterSongSearch::Util
{
    string SortToString(int sortMode) {
        switch (sortMode)
        {
        case 0:
            return "Newest";
            break;
        case 1:
            return "Oldest";
            break;
        case 2:
            return "Latest_Ranked";
            break;
        case 3:
            return "Most_Stars";
            break;
        case 4:
            return "Least_Stars";
            break;
        case 5:
            return "Best_rated";
            break;
        case 6:
            return "Worst_rated";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    string RequirementTypeToString(int requirementType) {
        switch (requirementType)
        {
        case 0:
            return "Any";
            break;
        case 1:
            return "NoodleExtensions";
            break;
        case 2:
            return "MappingExtensions";
            break;
        case 3:
            return "Chroma";
            break;
        case 4:
            return "Cinema";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    string CharFilterTypeToString(int charFilterType) {
        switch (charFilterType)
        {
        case 0:
            return "All";
            break;
        case 1:
            return "Custom";
            break;
        case 2:
            return "Standard";
            break;
        case 3:
            return "OneSaber";
            break;
        case 4:
            return "NoArrows";
            break;
        case 5:
            return "NinetyDegrees";
            break;
        case 6:
            return "ThreeSixtyDegrees";
            break;
        case 7:
            return "LightShow";
            break;
        case 8:
            return "Lawless";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    string DifficultyFilterTypeToString(int diffFilterType) {
        switch (diffFilterType)
        {
        case 0:
            return "All";
            break;
        case 1:
            return "Easy";
            break;
        case 2:
            return "Normal";
            break;
        case 3:
            return "Hard";
            break;
        case 4:
            return "Expert";
            break;
        case 5:
            return "ExpertPlus";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    string RankedFilterTypeToString(int rankedFilterType) {
        switch (rankedFilterType)
        {
        case 0:
            return "All";
            break;
        case 1:
            return "OnlyRanked";
            break;
        case 2:
            return "HideRanked";
            break;
        default:
            return "Unknown";
            break;
        }
    }
    string LocalScoreFilterTypeToString(int localScoreFilterType) {
        switch (localScoreFilterType)
        {
        case 0:
            return "All";
            break;
        case 1:
            return "HidePassed";
            break;
        case 2:
            return "OnlyPassed";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    string DownloadFilterTypeToString(int downloadFilterType) {
        switch (downloadFilterType)
        {
        case 0:
            return "All";
            break;
        case 1:
            return "HidePassed";
            break;
        case 2:
            return "OnlyPassed";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    // Logs song info for checking sort and options
    void LogSongInfo(const SDC_wrapper::BeatStarSong* song) {
        // DEBUG info
        song_data_core::UnixTime struct1RankedUpdateTime = 0;
        auto struct1DiffVec = song->GetDifficultyVector();
        for(auto const& i : struct1DiffVec)
        {
            DEBUG("Diff ranked time {}, unix {}", i->ranked_update_time.string_data, i->ranked_update_time_unix_epoch);
        
            struct1RankedUpdateTime = std::max( i->ranked_update_time_unix_epoch, struct1RankedUpdateTime);
        }
        DEBUG("Ranked time: {}", struct1RankedUpdateTime);
    }
}
