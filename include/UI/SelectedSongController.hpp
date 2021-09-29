#pragma once

#include "UnityEngine/MonoBehaviour.hpp"

#include "custom-types/shared/macros.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "HMUI/ImageView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"

DECLARE_CLASS_CODEGEN(BetterSongSearch::UI, SelectedSongController, UnityEngine::MonoBehaviour,
    public:
    void SetSong(const SDC_wrapper::BeatStarSong*);
    UnityEngine::UI::Button* playButton = nullptr;
    UnityEngine::UI::Button* downloadButton = nullptr;
    TMPro::TextMeshProUGUI* authorText = nullptr;
    TMPro::TextMeshProUGUI* songNameText = nullptr;
    TMPro::TextMeshProUGUI* dateText = nullptr;
)