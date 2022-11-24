#pragma once

#include "HMUI/TextSegmentedControl.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "overrides/CustomTextSegmentedControlData.hpp"
#include "custom-types/shared/delegate.hpp"
#include "System/Action_2.hpp"

template<typename T, typename U>
concept QuestUIConvertible = std::is_convertible_v<T, U>;

template<typename T>
concept HasTransform = requires (T a) { {a->get_transform() } -> QuestUIConvertible<UnityEngine::Transform*>; };

template<typename T>
concept HasGameObject = requires (T a) { {a->get_gameObject() } -> QuestUIConvertible<UnityEngine::GameObject*>; };


using namespace UnityEngine;

// Bug in quest UI that was fixed in nya
namespace QUIOverride {
    CustomTextSegmentedControlData* CreateTextSegmentedControl(UnityEngine::Transform* parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, ArrayW<StringW> values, std::function<void(int)> onCellWithIdxClicked);
    /// @brief creates a text segmented control like the one on the gameplay setup view controller
    /// @param parent what to parent it to
    /// @param anchoredPosition the position
    /// @param sizeDelta the sizeDelta
    /// @param values list of text values to give to the controller
    /// @param onCellWithIdxClicked callback called when a cell is clicked
    /// @return the created text segmented control
    CustomTextSegmentedControlData* CreateTextSegmentedControl(UnityEngine::Transform* parent, UnityEngine::Vector2 anchoredPosition, UnityEngine::Vector2 sizeDelta, ArrayW<StringW> values, std::function<void(int)> onCellWithIdxClicked = nullptr);
    
    /// @brief creates a text segmented control like the one on the gameplay setup view controller
    /// @param parent what to parent it to
    /// @param sizeDelta the sizeDelta
    /// @param values list of text values to give to the controller
    /// @param onCellWithIdxClicked callback called when a cell is clicked
    /// @return the created text segmented control
    CustomTextSegmentedControlData* CreateTextSegmentedControl(UnityEngine::Transform* parent, UnityEngine::Vector2 sizeDelta, ArrayW<StringW> values, std::function<void(int)> onCellWithIdxClicked = nullptr);
    
    /// @brief creates a text segmented control like the one on the gameplay setup view controller
    /// @param parent what to parent it to
    /// @param values list of text values to give to the controller
    /// @param onCellWithIdxClicked callback called when a cell is clicked
    /// @return the created text segmented control
    CustomTextSegmentedControlData* CreateTextSegmentedControl(UnityEngine::Transform* parent, ArrayW<StringW> values, std::function<void(int)> onCellWithIdxClicked);

    /// @brief creates a text segmented control like the one on the gameplay setup view controller
    /// @param parent what to parent it to
    /// @param onCellWithIdxClicked callback called when a cell is clicked
    /// @return the created text segmented control
    CustomTextSegmentedControlData* CreateTextSegmentedControl(UnityEngine::Transform* parent, std::function<void(int)> onCellWithIdxClicked);
    
    /// @brief Overload for creating a text segmented control that allows you to pass in anything that has a ->get_transform() method for the parent
    /// @param parent what to parent it to
    /// @return the created text segmented control
    template<HasTransform T, typename ...TArgs>
    requires(!std::is_convertible_v<T, UnityEngine::Transform*>)
    inline CustomTextSegmentedControlData* CreateTextSegmentedControl(T parent, TArgs...args) {
        return CreateTextSegmentedControl(parent->get_transform(), args...);
    }
}