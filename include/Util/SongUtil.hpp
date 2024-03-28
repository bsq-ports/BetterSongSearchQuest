#pragma once

#include "System/IO/Path.hpp"
#include "System/IO/File.hpp"
#include "System/String.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Sprite.hpp"
#include "songcore/shared/SongCore.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "song-details/shared/Data/RankedStates.hpp"
#include "song-details/shared/Data/SongDifficulty.hpp"
#include "FilterOptions.hpp"
#include "UI/ViewControllers/SongList.hpp"


namespace BetterSongSearch::Util {
    UnityEngine::Sprite* getLocalCoverSync(SongCore::SongLoader::CustomBeatmapLevel* level);


    UnityEngine::Sprite* getLocalCoverSync(StringW songHash);

    SongDetailsCache::RankedStates GetTargetedRankLeaderboardService(const SongDetailsCache::SongDifficulty* diff);

    float getStars(const SongDetailsCache::SongDifficulty* diff, SongDetailsCache::RankedStates state);

    float getStars(const SongDetailsCache::SongDifficulty* diff);
}
