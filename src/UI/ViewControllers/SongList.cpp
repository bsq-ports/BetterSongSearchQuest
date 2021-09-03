#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"

#include "UnityEngine/UI/ContentSizeFitter.hpp"
using namespace QuestUI;

#include "config-utils/shared/config-utils.hpp"

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListViewController);

UnityEngine::UI::VerticalLayoutGroup* songListLayout;

void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    songListLayout = BeatSaberUI::CreateVerticalLayoutGroup(get_rectTransform());

    UnityEngine::UI::LayoutElement* songListElement = songListLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    songListLayout->set_childAlignment(UnityEngine::TextAnchor::UpperCenter);
    songListLayout->set_childControlWidth(true);
    songListLayout->set_childControlHeight(true);
    songListElement->set_preferredWidth(120);


    auto backgroundSongList = songListLayout->get_gameObject()->AddComponent<Backgroundable*>();
    backgroundSongList->ApplyBackground(il2cpp_utils::newcsstr("round-rect-panel"));

    //BeatSaberUI::CreateText(songListLayout->get_rectTransform(), "Song List");

    UnityEngine::UI::HorizontalLayoutGroup* filterContainer = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(songListLayout->get_rectTransform());
    filterContainer->set_childAlignment(UnityEngine::TextAnchor::UpperLeft);
    filterContainer->set_childForceExpandWidth(false);
    filterContainer->set_childControlWidth(false);
    filterContainer->get_gameObject()->AddComponent<Backgroundable*>();
    auto background = filterContainer->get_gameObject()->AddComponent<Backgroundable*>();
    background->ApplyBackground(il2cpp_utils::newcsstr("round-rect-panel"));
    UnityEngine::UI::LayoutElement* element = filterContainer->GetComponent<UnityEngine::UI::LayoutElement*>();
    element->set_preferredHeight(10);

    std::function<void()> randomButtonOnClick = [=]() {

    };

    auto randomButton = BeatSaberUI::CreateUIButton(filterContainer->get_rectTransform(), "RANDOM", randomButtonOnClick);
}