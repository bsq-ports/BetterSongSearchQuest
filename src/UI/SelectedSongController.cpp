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
        getLogger().info("No Method: %s", song->hash.string_data);
        getLogger().info("Method: %s", song->GetHash().data());
        BeatSaver::API::GetBeatmapByHashAsync(std::string(song->GetHash()), 
        [this](std::optional<BeatSaver::Beatmap> beatmap)
        {
            if(beatmap.has_value()) {
                BeatSaver::API::GetCoverImageAsync(beatmap.value(),
                    [this](std::vector<uint8_t> result) {
                    auto arr = il2cpp_utils::vectorToArray(result);
                    getLogger().info("Downloaded");
                    QuestUI::MainThreadScheduler::Schedule([this, arr]
                    {
                        this->coverImage->set_sprite(QuestUI::BeatSaberUI::ArrayToSprite(arr));
                        this->coverImage->get_rectTransform()->set_sizeDelta(UnityEngine::Vector2(160, 160));
                    });
                });
            }
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
    stream4 << std::fixed << std::setprecision(2) << maxNJS;
    std::string maxNJSStr = stream4.str();
    infoText->set_text(il2cpp_utils::newcsstr(minNPSStr + std::string(" - ") + maxNPSStr + std::string(" NPS \n ") + minNJSStr + std::string(" - ") + maxNJSStr + std::string(" NJS")));
    songNameText->set_text(il2cpp_utils::newcsstr(song->GetName()));
    authorText->set_text(il2cpp_utils::newcsstr(song->GetSongAuthor()));
}

float roundFloat(float value)
{
    return round(value*10)/10;
}

void BetterSongSearch::UI::SelectedSongController::DownloadSong()
{
    downloadButton->set_interactable(false);
    std::function<void(float)> progressUpdate = [=](float downloadPercentage) {
        getLogger().info("DownloadProgress: %f", roundFloat(downloadPercentage));
        std::string value1 = std::to_string(roundFloat(downloadPercentage));
        std::string value2;
        if(downloadPercentage < 10)
            value2 = value1.substr(0, 3);
        else
            value2 = value1.substr(0, 4);
        auto textMeshPro = downloadButton->get_gameObject()->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if(textMeshPro) {
            if(downloadPercentage >= 100) {
                textMeshPro->set_text(il2cpp_utils::newcsstr("Download"));
                playButton->get_gameObject()->set_active(true);
            }
            else
                textMeshPro->set_text(il2cpp_utils::newcsstr(value2 + "%"));
        }
    };

    BeatSaver::API::GetBeatmapByHashAsync(std::string(currentSong->GetHash()), 
    [=](std::optional<BeatSaver::Beatmap> beatmap)
    {
        if(beatmap.has_value())
        {
            BeatSaver::API::DownloadBeatmapAsync(beatmap.value(),
            [=](bool error) {
                if (!error) {
                    QuestUI::MainThreadScheduler::Schedule(
                        [=] {
                            RuntimeSongLoader::API::RefreshSongs(false);
                            downloadButton->set_interactable(true);
                            downloadButton->get_gameObject()->set_active(false);
                        }
                    );
                }
                else {
                    auto textMeshPro = downloadButton->get_gameObject()->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
                    textMeshPro->set_text(il2cpp_utils::newcsstr("Download"));
                    downloadButton->set_interactable(true);
                }
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
    HMUI::IconSegmentedControl* tabSelector = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("HorizontalIconSegmentedControl"))->GetComponent<HMUI::IconSegmentedControl*>();
    HMUI::IconSegmentedControlCell* customLevelsTab = tabSelector->get_transform()->GetChild(2)->get_gameObject()->GetComponent<HMUI::IconSegmentedControlCell*>();
    customLevelsTab->SetSelected(true, HMUI::SelectableCell::TransitionType::Instant, customLevelsTab, true);
        tabSelector->SelectCellWithNumber(2);
    UnityEngine::GameObject* levelTable = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("LevelsTableView"));
    levelTable->GetComponent<GlobalNamespace::LevelCollectionTableView*>()->SelectLevel(level);
}

void BetterSongSearch::UI::SelectedSongController::PlaySong()
{
    /*GlobalNamespace::MainFlowCoordinator* mfc = QuestUI::BeatSaberUI::GetMainFlowCoordinator();//UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::MainFlowCoordinator*>()->values[0];
    auto sfpfc = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SoloFreePlayFlowCoordinator*>()->values[0];
    
    auto level = RuntimeSongLoader::API::GetLevelByHash(std::string(currentSong->GetHash()));
    if(level.has_value())
    { 
        currentLevel = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(level.value());
    }
    inBSS = true;
    fcInstance->PresentFlowCoordinator(sfpfc, nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false);*/
    backButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("BackButton"));
    auto level = RuntimeSongLoader::API::GetLevelByHash(std::string(currentSong->GetHash()));
    if(level.has_value())
    { 
        currentLevel = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(level.value());
    }
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(reinterpret_cast<custom_types::Helpers::enumeratorT*>(custom_types::Helpers::CoroutineHelper::New(EnterSolo(currentLevel))));
}