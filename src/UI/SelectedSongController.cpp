#include "main.hpp"
#include "UI/SelectedSongController.hpp"
#include "BeatSaverRegionManager.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/SongPreviewPlayer.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/IconSegmentedControlCell.hpp"
#include "HMUI/SelectableCell.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"

#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/AudioType.hpp"
#include "UnityEngine/Networking/DownloadHandlerAudioClip.hpp"
#include "UnityEngine/Networking/UnityWebRequestMultimedia.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "fmt/fmt/include/fmt/core.h"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

#include <iomanip>
#include <sstream>
#include <map>
//For when doing stuff on versions where song downloader doesnt exist. Saves a bit of time
#define SONGDOWNLOADER

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

inline std::string toLower(std::string_view s) {
    return toLower(std::string(s));
}

inline std::string toLower(char const* s) {
    return toLower(std::string(s));
}

void BetterSongSearch::UI::SelectedSongController::SetSong(const SDC_wrapper::BeatStarSong* song)
{
    currentSong = song;

    update();
}

void BetterSongSearch::UI::SelectedSongController::DownloadSong()
{
    #ifdef SONGDOWNLOADER

    auto songData = this->currentSong.getData();
    if (songData != nullptr) {
        fcInstance->DownloadHistoryViewController->TryAddDownload(songData);
        downloadButton->interactable = false;
        downloadButton.update();
    }  else {
        getLogger().warning("Current song is null, doing nothing");
    }
    
    // std::function<void(float)> progressUpdate = [this](float downloadPercentage) {
    //     fmtLog(Logging::Level::INFO, "DownloadProgress: {0:.2f}", downloadPercentage);

    //     if (downloadPercentage <= 100) {
    //         downloadButton->text.text = fmt::format("{:.2f}%", downloadPercentage);
    //         QuestUI::MainThreadScheduler::Schedule([this] {
    //             downloadButton.update();
    //         });
    //     }
    // };

    // BeatSaver::API::GetBeatmapByHashAsync(std::string(currentSong->GetHash()),
    // [this, progressUpdate](std::optional<BeatSaver::Beatmap> beatmap)
    // {
    //     if(beatmap.has_value())
    //     {
    //         BeatSaver::API::DownloadBeatmapAsync(beatmap.value(),
    //         [this](bool error) {
    //             QuestUI::MainThreadScheduler::Schedule(
    //                     [error, this] {
    //                         downloadButton->interactable = error;
    //                         downloadButton->active = error;

    //                         playButton->active = !error;
    //                         playButton->interactable = !error;

    //                         if (error) {
    //                             downloadButton->text.text = "Download";
    //                         } else {
    //                             RuntimeSongLoader::API::RefreshSongs(false);
    //                         }

    //                         downloadButton.update();
    //                         playButton.update();
    //                     }
    //             );
    //         }, progressUpdate);
    //     }
    // });
    #endif
}

UnityEngine::GameObject* backButton = nullptr;
UnityEngine::GameObject* soloButton = nullptr;
GlobalNamespace::SongPreviewPlayer* songPreviewPlayer = nullptr;
GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel = nullptr;
GlobalNamespace::LevelCollectionViewController* levelCollectionViewController = nullptr;

void BetterSongSearch::UI::SelectedSongController::DidActivate(bool firstActivation) {
    songPreviewPlayer = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SongPreviewPlayer*>().FirstOrDefault();
    levelCollectionViewController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionViewController*>().FirstOrDefault();
    beatmapLevelsModel = QuestUI::ArrayUtil::First(
            UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::BeatmapLevelsModel *>(),
            [](GlobalNamespace::BeatmapLevelsModel *x) {
                return x->customLevelPackCollection != nullptr;
            });
}

custom_types::Helpers::Coroutine EnterSolo(GlobalNamespace::IPreviewBeatmapLevel* level) {
    backButton->GetComponent<UnityEngine::UI::Button *>()->Press();
    co_yield reinterpret_cast<System::Collections::IEnumerator *>(CRASH_UNLESS(UnityEngine::WaitForSeconds::New_ctor(0.5)));
    soloButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
    soloButton->GetComponent<HMUI::NoTransitionsButton *>()->Press();
    GlobalNamespace::LevelCollectionNavigationController* levelCollectionNavigationController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionNavigationController*>().FirstOrDefault();
    if(levelCollectionNavigationController) {
        co_yield reinterpret_cast<System::Collections::IEnumerator *>(CRASH_UNLESS(UnityEngine::WaitForSeconds::New_ctor(0.3)));
        levelCollectionNavigationController->SelectLevel(level);
    }
}

void BetterSongSearch::UI::SelectedSongController::PlaySong()
{
    backButton = UnityEngine::GameObject::Find("BackButton");
    auto level = RuntimeSongLoader::API::GetLevelByHash(std::string(currentSong->GetHash()));
    if(level.has_value())
    {
        currentLevel = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(level.value());
    }
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(EnterSolo(currentLevel)));

}

void GetByURLAsync(std::string url, std::function<void(std::vector<uint8_t>)> finished) {
    BeatSaverRegionManager::GetAsync(url,
           [finished](long httpCode, std::string data) {
               std::vector<uint8_t> bytes(data.begin(), data.end());
               finished(bytes);
           }, [](float progress){}
    );
}

custom_types::Helpers::Coroutine GetPreview(std::string url, std::function<void(UnityEngine::AudioClip*)> finished) {
    auto webRequest = UnityEngine::Networking::UnityWebRequestMultimedia::GetAudioClip(url, UnityEngine::AudioType::MPEG);
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(webRequest->SendWebRequest()));
    if(webRequest->get_isNetworkError())
        getLogger().info("Network error");

    while(webRequest->GetDownloadProgress() < 1.0f);

    getLogger().info("Download complete");
    UnityEngine::AudioClip* clip = UnityEngine::Networking::DownloadHandlerAudioClip::GetContent(webRequest);
    finished(clip);
}

void BetterSongSearch::UI::SelectedSongController::update() {
    if (!currentSong.getData()) {
        updateView();
        return;
    }

    // if same song, don't modify
    if (!currentSong.readAndClear(*ctx)) {
        updateView();
        return;
    }

    auto song = *currentSong;

    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->GetHash()));
    bool downloaded = beatmap.has_value();
    #ifdef SONGDOWNLOADER
    //getLogger().info("Gathering information...");
    coverImage.child.sprite = defaultImage;

    //getLogger().info("No Method: %s", song->hash.string_data);
    //getLogger().info("Method: %s", song->GetHash().data());

    if(this->imageCoverCache.contains(std::string(song->GetHash()))) {
        std::vector<uint8_t> data = this->imageCoverCache[std::string(song->GetHash())];
        Array<uint8_t>* spriteArray = il2cpp_utils::vectorToArray(data);
        this->coverImage.child.sprite = QuestUI::BeatSaberUI::ArrayToSprite(spriteArray);
        this->coverImage.child.sizeDelta = UnityEngine::Vector2(160, 160);
        this->coverImage.update();
    }
    else {
        std::string newUrl = fmt::format("{}/{}.jpg", BeatSaverRegionManager::coverDownloadUrl, toLower(song->GetHash()));
        coverImageLoading.enabled = true;
        GetByURLAsync(newUrl, [this, song](std::vector<uint8_t> bytes) {
            QuestUI::MainThreadScheduler::Schedule([this, bytes, song] {
                std::vector<uint8_t> data = bytes;

                if (song != this->currentSong.getData()) return;
                this->imageCoverCache[std::string(song->GetHash())] = {data.begin(), data.end()};
                Array<uint8_t> *spriteArray = il2cpp_utils::vectorToArray(data);
                this->coverImage.child.sprite = QuestUI::BeatSaberUI::ArrayToSprite(spriteArray);
                this->coverImage.child.sizeDelta = UnityEngine::Vector2(160, 160);
                this->coverImage.update();
                this->coverImageLoading.enabled = false;
                this->updateView();
            });
        });
    }

    if(downloaded) {
        //Get preview from beatmap
        if(!beatmapLevelsModel)
            return;
        if(!levelCollectionViewController)
            return;

        auto preview = beatmapLevelsModel->GetLevelPreviewForLevelId(beatmap.value()->levelID);
        if(preview)
            levelCollectionViewController->SongPlayerCrossfadeToLevelAsync(preview);
    }
    else {
        if(!songPreviewPlayer)
            return;

        auto ssp = songPreviewPlayer;

        std::string newUrl = fmt::format("{}/{}.mp3", BeatSaverRegionManager::coverDownloadUrl, toLower(song->GetHash()));

        GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(GetPreview(newUrl, [ssp](UnityEngine::AudioClip* clip) {
            ssp->CrossfadeTo(clip, -5, 0, clip->get_length(), nullptr);
        })));
    }

    #endif

    float minNPS = 500000, maxNPS = 0;
    float minNJS = 500000, maxNJS = 0;

    for (auto diff: song->GetDifficultyVector()) {
        float nps = (float) diff->notes / (float) song->duration_secs;
        float njs = diff->njs;
        minNPS = std::min(nps, minNPS);
        maxNPS = std::max(nps, maxNPS);

        minNJS = std::min(njs, minNJS);
        maxNJS = std::max(njs, maxNJS);
    }

    downloadButton->text.text = "Download";
    QuestUI::MainThreadScheduler::Schedule([this] {
        downloadButton.update();
    });
    playButton->active = downloaded;
    playButton->interactable = downloaded;
    downloadButton->active = !downloaded;
    downloadButton->interactable = !downloaded;
    infoButton->interactable = true;

    infoText.text = fmt::format("{:.2f} - {:.2f} NPS \n {:.2f} - {:.2f} NJS", minNPS, maxNPS, minNJS, maxNJS);
    songNameText.text = song->GetName();
    authorText.text = song->GetSongAuthor();

    updateView();
}
