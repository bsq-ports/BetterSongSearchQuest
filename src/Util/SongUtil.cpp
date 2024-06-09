#include "Util/SongUtil.hpp"
#include "logging.hpp"
#include "bsml/shared/BSML-Lite/Creation/Image.hpp"

#include "UI/ViewControllers/SongList.hpp"
#include "songcore/shared/SongLoader/CustomBeatmapLevel.hpp"
namespace BetterSongSearch::Util {
    UnityEngine::Sprite* getLocalCoverSync(SongCore::SongLoader::CustomBeatmapLevel* level) {
        if (level == nullptr) {
            return nullptr;
        }
        
        std::string imageFileName = "";
        {
            auto saveDataV2 = level->get_standardLevelInfoSaveDataV2();
            if (saveDataV2.has_value()) {
                auto name = saveDataV2.value()->get_coverImageFilename();
                imageFileName = static_cast<std::string>(name);
            }
        }
        {
            if (imageFileName.empty()) {
                auto saveData = level->get_beatmapLevelSaveDataV4();
                if (saveData.has_value()) {
                    auto name = saveData.value()->__cordl_internal_get_coverImageFilename();
                    imageFileName = static_cast<std::string>(name);
                }
            }
        }
        if (imageFileName.empty()) {
            DEBUG("No cover image found in level data");
            return nullptr;
        }

        StringW path = System::IO::Path::Combine(
                level->get_customLevelPath(),
        imageFileName);

        if(!System::IO::File::Exists(path)) {
            DEBUG("File does not exist");
            return nullptr;
        }

        auto sprite = BSML::Lite::FileToSprite((std::string) path);
        return sprite;
    }


    UnityEngine::Sprite* getLocalCoverSync(StringW songHash) {
        auto beatmap = SongCore::API::Loading::GetLevelByHash(std::string(songHash));

        if (beatmap) {
            return getLocalCoverSync(beatmap);
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
