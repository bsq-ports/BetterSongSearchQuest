#pragma once
#include "Util/SongUtil.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "UI/ViewControllers/SongList.hpp"

namespace BetterSongSearch::Util {
    UnityEngine::Sprite* getLocalCoverSync(GlobalNamespace::CustomPreviewBeatmapLevel* level) {
        StringW path = System::IO::Path::Combine(level->customLevelPath, level->standardLevelInfoSaveData->coverImageFilename);

        if(!System::IO::File::Exists(path)) {
            DEBUG("File does not exist");
            return nullptr;
        }

        auto sprite = QuestUI::BeatSaberUI::FileToSprite((std::string) path);
        return sprite;
    }


    UnityEngine::Sprite* getLocalCoverSync(StringW songHash) {
        auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(songHash));

        if (beatmap.has_value()) {
            return getLocalCoverSync(beatmap.value());
        } else {
            return nullptr;
        }
    }

    // Gets preferred leaderboard for a song difficulty
    SongDetailsCache::RankedStates GetTargetedRankLeaderboardService(const SongDetailsCache::SongDifficulty* diff) {
        auto& rStates = diff->song().rankedStates;

        // If song is scoresaber ranked
        if (hasFlags(rStates, SongDetailsCache::RankedStates::ScoresaberRanked) && 
            // And Not Filtering by BeatLeader ranked
            UI::DataHolder::filterOptionsCache.rankedType != FilterOptions::RankedFilterType::BeatLeaderRanked && 
            (
                // Beatleader is not preferred leaderboard
                BetterSongSearch::UI::DataHolder::preferredLeaderboard != PreferredLeaderBoard::BeatLeader ||
                // Song has no BeatLeader rank
                !hasFlags(rStates, SongDetailsCache::RankedStates::BeatleaderRanked) ||
                // Filtering by SS ranked
                UI::DataHolder::filterOptionsCache.rankedType == FilterOptions::RankedFilterType::ScoreSaberRanked 
            )
        ) {
            return SongDetailsCache::RankedStates::ScoresaberRanked;
        }

        // If song has beatleader rank then return beatleader 
        if (hasFlags(rStates, SongDetailsCache::RankedStates::BeatleaderRanked)) {
            return SongDetailsCache::RankedStates::BeatleaderRanked;
        }

        return SongDetailsCache::RankedStates::Unranked;
    }

    float getStars(const SongDetailsCache::SongDifficulty* diff, SongDetailsCache::RankedStates state) {
        if (state == SongDetailsCache::RankedStates::ScoresaberRanked && diff->starsSS > 0) {
            return diff->starsSS;
        }
        if (state == SongDetailsCache::RankedStates::BeatleaderRanked && diff->starsBL > 0) {
            return diff->starsBL;
        }
        return 0;
    }

    float getStars(const SongDetailsCache::SongDifficulty* diff) {
        return getStars(diff, GetTargetedRankLeaderboardService(diff));
    }
}
