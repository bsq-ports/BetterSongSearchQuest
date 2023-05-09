#include "Util/Debug.hpp"
#include <string>
#include "main.hpp"
#include "UI/ViewControllers/SongList.hpp"

using namespace std;


    string BetterSongSearch::Util::SortToString(int sortMode) {
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

    string BetterSongSearch::Util::RequirementTypeToString(int requirementType) {
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
        case 5:
            return "None";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    string BetterSongSearch::Util::CharFilterTypeToString(int charFilterType) {
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

    string BetterSongSearch::Util::DifficultyFilterTypeToString(int diffFilterType) {
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

    string BetterSongSearch::Util::RankedFilterTypeToString(int rankedFilterType) {
        switch (rankedFilterType)
        {
        case 0:
            return "ShowAll";
            break;
        case 1:
            return "ScoreSaberRanked";
            break;
        case 2:
            return "BeatLeaderRanked";
            break;
        case 3:
            return "ScoreSaberQualified";
            break;
        case 4:
            return "BeatLeaderQualified";
            break;
        default:
            return "Unknown";
            break;
        }
    }
    string BetterSongSearch::Util::LocalScoreFilterTypeToString(int localScoreFilterType) {
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

    string BetterSongSearch::Util::DownloadFilterTypeToString(int downloadFilterType) {
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
    void BetterSongSearch::Util::LogSongInfo(const SongDetailsCache::Song* song) {
        // DEBUG info
        auto struct1DiffVec = song->rankedChangeUnix;
        DEBUG("Name: {}, bpm {}, upvotes: {}, downvotes: {}, diffCount: {}, rankedStatus: {}, rankedStates: {} ", (std::string)song->songName(), song->bpm, song->upvotes, song->downvotes, song->diffCount, song->rankedStatus, song->rankedStates  );
        DEBUG("Ranked time: {}", struct1DiffVec);
        for (const SongDetailsCache::SongDifficulty &diff: *song) {
            DEBUG(
                "Diff: {}, Ranked: {}, starsSS: {}, starsBL: {}, notes: {}, characteristic: {}, njs: {}, mods: {} ",
                diff.difficulty, diff.ranked(), diff.starsSS, diff.starsBL, diff.notes, diff.characteristic, diff.njs, diff.mods
            );

        }
        DEBUG("Sort score: {}", sortFunctionMap.at(UI::DataHolder::currentSort)(song));
    }

    // Prints to the provided buffer a nice number of bytes (KB, MB, GB, etc)
    string BetterSongSearch::Util::pretty_bytes(int64_t bytes)
    {
        string prefix = "";

        if (bytes < 0 ) {
            prefix = "-";
        }
        const char* suffixes[7];
        suffixes[0] = "B";
        suffixes[1] = "KB";
        suffixes[2] = "MB";
        suffixes[3] = "GB";
        suffixes[4] = "TB";
        suffixes[5] = "PB";
        suffixes[6] = "EB";
        uint s = 0; // which suffix to use
        double count = abs(bytes);
        while (count >= 1024 && s < 7)
        { 
            s++;
            count /= 1024;
        }
        if (count - floor(count) == 0.0)
            return fmt::format("{}{} {}", prefix, (int)count, suffixes[s]);
        else
            return fmt::format("{}{:.2f} {}", prefix, count, suffixes[s]);
    }

