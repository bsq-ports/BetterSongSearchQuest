#pragma once

#include "System/Action.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/Coroutine.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"
#include "HMUI/VerticalScrollIndicator.hpp"
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/StringSetting.hpp"
using namespace UnityEngine;

namespace BetterSongSearch::UI::Util::BSMLStuff {
    custom_types::Helpers::Coroutine MergeSliders(GameObject* container, bool constrictValuesMinMax = true);

    void SetStringSettingValue(BSML::StringSetting* element, std::string value);
    void SetSliderSettingValue(BSML::SliderSetting* element, float value);
    void FormatSliderSettingValue(BSML::SliderSetting* element);
    void FormatStringSettingValue(BSML::StringSetting* element);
}
