#include "main.hpp"
#include "UI/SelectedSongController.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "questui/shared/ArrayUtil.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/IconSegmentedControlCell.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/SelectableCell.hpp"
#include "System/Action.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "System/Nullable_1.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator_State.hpp"
#include <iomanip>
#include <fmt/core.h>


void BetterSongSearch::UI::SelectedSongController::SetSong(const SDC_wrapper::BeatStarSong* song)
{
    // TODO: Move this to render()
    currentSong = song;
    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->GetHash()));
    bool downloaded = beatmap.has_value();

    getLogger().info("Downloading");
    coverImage.child.sprite = defaultImage;

    getLogger().info("No Method: %s", song->hash.string_data);
    getLogger().info("Method: %s", song->GetHash().data());
    BeatSaver::API::GetBeatmapByHashAsync(std::string(song->GetHash()),
    [this](std::optional<BeatSaver::Beatmap> beatmap)
    {
        if(beatmap.has_value()) {
            std::vector<uint8_t> result = BeatSaver::API::GetCoverImage(beatmap.value());
            auto arr = il2cpp_utils::vectorToArray(result);
            getLogger().info("Downloaded");
            QuestUI::MainThreadScheduler::Schedule([this, arr]
            {
                this->coverImage.child.sprite = QuestUI::BeatSaberUI::ArrayToSprite(arr);
                this->coverImage.child.sizeDelta = {160, 160};
                this->coverImage.update();
            });
        }
    });

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

    playButton.child.enabled = downloaded;
    downloadButton.child.enabled = !downloaded;

    infoText.child.text = fmt::format("{0:.2f} - {0:.2f} NPS \n {0:.2f} - {0:.2f} NJS", minNPS, maxNPS, minNJS, maxNJS);
    songNameText.child.text = song->GetName();
    authorText.child.text = song->GetSongAuthor();

    QUC::detail::renderSingle(*this, *ctx);
}

void BetterSongSearch::UI::SelectedSongController::DownloadSong()
{
    downloadButton.child.interactable = false;
    std::function<void(float)> progressUpdate = [=](float downloadPercentage) {
        fmtLog(Logging::Level::INFO, "DownloadProgress: {0:.4f}", downloadPercentage);

        if(downloadPercentage >= 100) {
            downloadButton.child.text.text = "Download";
            downloadButton.child.enabled = false;
            playButton.child.enabled = true;


            playButton.update();
            downloadButton.update();
        }
        else {
            downloadButton.child.text.text = fmt::format("{0:.4f}%", downloadPercentage);
            downloadButton.update();
        }
    };

    BeatSaver::API::GetBeatmapByHashAsync(std::string(currentSong->GetHash()),
    [=](std::optional<BeatSaver::Beatmap> beatmap)
    {
        if(beatmap.has_value())
        {
            BeatSaver::API::DownloadBeatmapAsync(beatmap.value(),
            [=](bool error) {
                QuestUI::MainThreadScheduler::Schedule(
                        [=] {
                            if (!error) {
                                RuntimeSongLoader::API::RefreshSongs(false);
                                downloadButton.child.interactable = true;
                                downloadButton.child.enabled = false;
                                downloadButton.update();
                            } else {
                                downloadButton.child.text.text = "Download";
                                downloadButton.child.interactable = true;
                                downloadButton.update();
                            }
                        }
                );
            }, progressUpdate);
        }
    });
}

UnityEngine::GameObject* backButton = nullptr;
UnityEngine::GameObject* soloButton = nullptr;

custom_types::Helpers::Coroutine EnterSolo(GlobalNamespace::IPreviewBeatmapLevel* level) {
    backButton->GetComponent<UnityEngine::UI::Button*>()->Press();
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(UnityEngine::WaitForSeconds::New_ctor(0.5)));
    soloButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
    soloButton->GetComponent<UnityEngine::UI::Button*>()->Press();
    HMUI::IconSegmentedControl* tabSelector = UnityEngine::GameObject::Find("HorizontalIconSegmentedControl")->GetComponent<HMUI::IconSegmentedControl*>();
    HMUI::IconSegmentedControlCell* customLevelsTab = tabSelector->get_transform()->GetChild(2)->get_gameObject()->GetComponent<HMUI::IconSegmentedControlCell*>();
    customLevelsTab->SetSelected(true, HMUI::SelectableCell::TransitionType::Instant, customLevelsTab, true);
        tabSelector->SelectCellWithNumber(2);
    UnityEngine::GameObject* levelTable = UnityEngine::GameObject::Find("LevelsTableView");
    levelTable->GetComponent<GlobalNamespace::LevelCollectionTableView*>()->SelectLevel(level);
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