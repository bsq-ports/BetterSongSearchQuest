#include "UI/ViewControllers/FilterView.hpp"
#include "main.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
using namespace QuestUI;

#include "config-utils/shared/config-utils.hpp"

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

UnityEngine::UI::VerticalLayoutGroup* filterViewLayout;

void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    filterViewLayout = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());

    BeatSaberUI::CreateText(filterViewLayout->get_rectTransform(), "Filters");
}