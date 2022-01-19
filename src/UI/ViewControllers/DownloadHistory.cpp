#include "UI/ViewControllers/DownloadHistory.hpp"
#include "main.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
using namespace QuestUI;

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController);

UnityEngine::UI::VerticalLayoutGroup* downloadHistoryLayout;

void ViewControllers::DownloadHistoryViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    downloadHistoryLayout = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
}