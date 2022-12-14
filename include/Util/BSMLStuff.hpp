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

using namespace UnityEngine;

namespace BetterSongSearch::UI::Util::BSMLStuff {
    custom_types::Helpers::Coroutine MergeSliders(GameObject* container, bool constrictValuesMinMax = true);
}
