#pragma once

#include "UnityEngine/MonoBehaviour.hpp"

#include "custom-types/shared/macros.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Sprite.hpp"
#include "HMUI/ImageView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "questui/shared/CustomTypes/Components/Settings/SliderSetting.hpp"


DECLARE_CLASS_CODEGEN(BetterSongSearch::UI, SliderFormatter, UnityEngine::MonoBehaviour,
    public:
    std::function<std::string(float)> formatFunction;
)