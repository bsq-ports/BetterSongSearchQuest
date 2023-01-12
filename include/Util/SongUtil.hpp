#pragma once

#include "System/IO/Path.hpp"
#include "System/IO/File.hpp"
#include "System/String.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Sprite.hpp"
#include "songloader/shared/API.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"

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
    
}
