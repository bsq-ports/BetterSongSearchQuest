#include "UI/ViewControllers/DownloadHistory.hpp"
#include "main.hpp"


#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/TableView.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"

#include "bsml/shared/BSML.hpp"
#include "assets.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController);

void ViewControllers::DownloadHistoryViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    getLogger().info("Download contoller activated");
    BSML::parse_and_construct(IncludedAssets::DownloadHistory_bsml, this->get_transform(), this);

    if (this->downloadList != nullptr && this->downloadList->m_CachedPtr.m_value != nullptr ) { 
        getLogger().info("Table exists");
        // this->downloadList->data = 
    }
}

void  ViewControllers::DownloadHistoryViewController::SelectSong(HMUI::TableView* table, int id) {
    getLogger().info("Cell clicked %i", id);
}

// BSML::CustomCellInfo