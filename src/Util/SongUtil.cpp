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

    SongDetailsCache::RankedStates GetTargetedRankLeaderboardService(const SongDetailsCache::SongDifficulty* diff) {
        auto& rStates = diff->song().rankedStates;

        if (hasFlags(rStates, SongDetailsCache::RankedStates::ScoresaberRanked) && 
            // Not Filtering by BeatLeader ranked
            UI::DataHolder::filterOptionsCache.rankedType != FilterOptions::RankedFilterType::BeatLeaderRanked && 
            
            (
                // Not filtering by BeatLeader
                BetterSongSearch::UI::DataHolder::preferredLeaderboard != PreferredLeaderBoard::BeatLeader ||
                // Song has no beatleader rank
                !hasFlags(rStates, SongDetailsCache::RankedStates::BeatleaderRanked) ||
                // Filtering by SS ranked
                UI::DataHolder::filterOptionsCache.rankedType != FilterOptions::RankedFilterType::ScoreSaberRanked 
            )
            

        ) {
            return SongDetailsCache::RankedStates::ScoresaberRanked;
        }

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
