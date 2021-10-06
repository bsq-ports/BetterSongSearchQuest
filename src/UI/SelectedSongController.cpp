#include "main.hpp"
#include "UI/SelectedSongController.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "questui/shared/ArrayUtil.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "System/Action.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"
#include <iomanip>
#include <sstream>

DEFINE_TYPE(BetterSongSearch::UI, SelectedSongController);

void BetterSongSearch::UI::SelectedSongController::SetSong(const SDC_wrapper::BeatStarSong* song)
{
    currentSong = song;
    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->GetHash()));
    bool downloaded = beatmap.has_value();
    //if(downloaded)
    //{
    //    coverImage->set_sprite(beatmap.value()->coverImage);
    //}
    //else
    //{
        getLogger().info("Downloading");
        coverImage->set_sprite(defaultImage);
        //):
        BeatSaver::API::GetBeatmapByHashAsync(std::string(song->GetHash()), 
        [this](std::optional<BeatSaver::Beatmap> beatmap)
        {
            BeatSaver::API::GetCoverImageAsync(beatmap.value(), 
            [this](std::vector<uint8_t> result) {
                auto arr = il2cpp_utils::vectorToArray(result);
                getLogger().info("Downloaded");
                QuestUI::MainThreadScheduler::Schedule([this, arr]
                {
                    this->coverImage->set_sprite(QuestUI::BeatSaberUI::ArrayToSprite(arr));
                });
            });
        });
    //}

    float minNPS = 500000, maxNPS = 0;
    float minNJS = 500000, maxNJS = 0;

    for(auto diff : song->GetDifficultyVector())
    {
        float nps = (float)diff->notes / (float)song->duration_secs;
        float njs = diff->njs;
        minNPS = std::min(nps, minNPS);
        maxNPS = std::max(nps, maxNPS);

        minNJS = std::min(njs, minNJS);
        maxNJS = std::max(njs, maxNJS);
    }

    playButton->get_gameObject()->SetActive(downloaded);
    downloadButton->get_gameObject()->SetActive(!downloaded);

    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << minNPS;
    std::string minNPSStr = stream.str();

    std::stringstream stream2;
    stream2 << std::fixed << std::setprecision(2) << maxNPS;
    std::string maxNPSStr = stream2.str();

    std::stringstream stream3;
    stream3 << std::fixed << std::setprecision(2) << minNJS;
    std::string minNJSStr = stream3.str();

    std::stringstream stream4;
    stream2 << std::fixed << std::setprecision(2) << maxNJS;
    std::string maxNJSStr = stream4.str();
    infoText->set_text(il2cpp_utils::newcsstr(minNPSStr + std::string(" - ") + maxNPSStr + std::string(" NPS / ") + minNJSStr + std::string(" - ") + maxNJSStr + std::string(" NJS")));
    songNameText->set_text(il2cpp_utils::newcsstr(song->GetName()));
    authorText->set_text(il2cpp_utils::newcsstr(song->GetSongAuthor()));
}

void BetterSongSearch::UI::SelectedSongController::DownloadSong()
{
    BeatSaver::API::GetBeatmapByHashAsync(std::string(currentSong->GetHash()), 
    [this](std::optional<BeatSaver::Beatmap> beatmap)
    {
        if(beatmap.has_value())
        {
            BeatSaver::API::DownloadBeatmapAsync(beatmap.value(),
            [this](bool error) {
                if (!error) {
                    QuestUI::MainThreadScheduler::Schedule(
                        [] {
                            RuntimeSongLoader::API::RefreshSongs(false);
                        }
                    );
                }
            }, nullptr);
        }
    });
}

std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); } // correct
                  );
    return s;
}

custom_types::Helpers::Coroutine coroutine(const SDC_wrapper::BeatStarSong* currentSong) {

    //co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(UnityEngine::WaitForEndOfFrame::New_ctor()));

    GlobalNamespace::LevelFilteringNavigationController* levelFilteringNavigationController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelFilteringNavigationController*>()->values[0];
    levelFilteringNavigationController->UpdateCustomSongs();
    levelFilteringNavigationController->UpdateSecondChildControllerContent(4);

    //co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(UnityEngine::WaitForEndOfFrame::New_ctor()));

    auto level = RuntimeSongLoader::API::GetLevelByHash(std::string(currentSong->GetHash()));
    if(level.has_value())
    { 
        GlobalNamespace::LevelCollectionNavigationController* levelCollectionNavigationController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionNavigationController*>()->values[0];
        if(!levelCollectionNavigationController)
            co_return;
        levelCollectionNavigationController->SelectLevel((GlobalNamespace::IPreviewBeatmapLevel*)(level.value()));
    }

    co_return;
}

void BetterSongSearch::UI::SelectedSongController::PlaySong()
{
    GlobalNamespace::MainFlowCoordinator* mfc = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MainFlowCoordinator*>()->values[0];
    auto sfpfc = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SoloFreePlayFlowCoordinator*>()->values[0];
    fcInstance->PresentFlowCoordinator(sfpfc, il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*), std::function([this](){
        GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(coroutine(this->currentSong))));
    })), HMUI::ViewController::AnimationDirection::Horizontal, false, false);
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(coroutine(this->currentSong))));
}