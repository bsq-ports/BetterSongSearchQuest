#include "UI/Modals/Settings.hpp"
#include "main.hpp"
#include "PluginConfig.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "bsml/shared/BSML.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"

#include "assets.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::Modals, Settings);

void Modals::Settings::OnEnable()
{
}


void Modals::Settings::CloseModal()
{
    this->settingsModal->Hide();
}

void Modals::Settings::ctor()
{
    INVOKE_CTOR();
    this->initialized = false;
}

void Modals::Settings::OpenModal()
{
    if (!initialized) {
        BSML::parse_and_construct(IncludedAssets::Settings_bsml, this->get_transform(), this);
        initialized = true;
    }
    this->settingsModal->Show();
}


bool Modals::Settings::get_returnToBssFromSolo() {    
    return getPluginConfig().ReturnToBSS.GetValue();
}
void Modals::Settings::set_returnToBssFromSolo(bool value) {
    getPluginConfig().ReturnToBSS.SetValue(value);
}


bool Modals::Settings::get_loadSongPreviews() {
   return getPluginConfig().LoadSongPreviews.GetValue();
}
void Modals::Settings::set_loadSongPreviews(bool value) {
    getPluginConfig().LoadSongPreviews.SetValue(value);
}

bool Modals::Settings::get_smallerFontSize() {
    return getPluginConfig().SmallerFontSize.GetValue();
}
void Modals::Settings::set_smallerFontSize(bool value) {
    getPluginConfig().SmallerFontSize.SetValue(value);
    fcInstance->SongListController->songListTable()->ReloadData();
}

StringW Modals::Settings::get_preferredLeaderboard() {
    // Preferred Leaderboard
    std::string preferredLeaderboard = getPluginConfig().PreferredLeaderboard.GetValue();
    if (leaderBoardMap.contains(preferredLeaderboard)) {
        return preferredLeaderboard;
    } else {
        DataHolder::preferredLeaderboard = PreferredLeaderBoard::ScoreSaber;
        getPluginConfig().PreferredLeaderboard.SetValue("Scoresaber");
        getPluginConfig().config->Write();
        return "Scoresaber";
    }
}

void Modals::Settings::set_preferredLeaderboard(StringW value) {
    std::string preferredLeaderboard = getPluginConfig().PreferredLeaderboard.GetValue();
    if (leaderBoardMap.contains(preferredLeaderboard)) {
        DataHolder::preferredLeaderboard = leaderBoardMap.at(preferredLeaderboard);
        getPluginConfig().PreferredLeaderboard.SetValue(value);
        getPluginConfig().config->Write();

        auto controller = fcInstance->SongListController;
        controller->filterChanged = true;
        controller->SortAndFilterSongs(controller->sort, controller->search, true);
    }
}
