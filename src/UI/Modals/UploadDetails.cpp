#include "UI/Modals/UploadDetails.hpp"
#include "main.hpp"
#include "PluginConfig.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "bsml/shared/BSML.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "BeatSaverRegionManager.hpp"
#include "assets.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

const std::vector<std::string> CHAR_GROUPING = {"Unknown", "Standard", "OneSaber", "NoArrows", "Lightshow", "NintyDegree", "ThreeSixtyDegree", "Lawless"};


DEFINE_TYPE(BetterSongSearch::UI::Modals, UploadDetails);

//name and count
std::unordered_map<std::string, int> GroupCharByDiff(std::vector<const SDC_wrapper::BeatStarSongDifficultyStats *> diffs) {
    std::unordered_map<std::string, int> groupedChars;
    for(auto diff : diffs) {
        auto key = CHAR_GROUPING[(int)diff->diff_characteristics];
        if(!groupedChars.contains(key))
            groupedChars.emplace(key, 1);
        else groupedChars[key] += 1;
    }
    return groupedChars;
}

void Modals::UploadDetails::OnEnable()
{
}


void Modals::UploadDetails::CloseModal()
{
    this->rootModal->Hide();
}

void Modals::UploadDetails::ctor()
{
    INVOKE_CTOR();
    this->initialized = false;
}

void Modals::UploadDetails::OpenModal(const SDC_wrapper::BeatStarSong* song)
{
    if (!initialized) {
        BSML::parse_and_construct(IncludedAssets::UploadDetails_bsml, this->get_transform(), this);
        initialized = true;
    }

    // Fill data
    // this->selec
    auto diffs = song->GetDifficultyVector();
    auto groupedDiffs = GroupCharByDiff(diffs);
    std::string characteristicsText = "";
    int loopCount = 1;
    for(auto& it: groupedDiffs) {
        if(loopCount < groupedDiffs.size())
            characteristicsText.append(fmt::format("{}x {}, ", it.second, it.first));
        else
            characteristicsText.append(fmt::format("{}x {}", it.second, it.first));
        loopCount++;
    }
    selectedCharacteristics->set_text(characteristicsText);
    selectedSongKey->set_text(song->key.string_data);
    selectedRating->set_text(fmt::format("{:.1f}%", song->rating * 100));
    selectedSongDescription->SetText("Loading...");

    songDetailsLoading->get_gameObject()->SetActive(true);

    BeatSaverRegionManager::GetSongDescription(std::string(song->key.string_data), [this](std::string value) {
        selectedSongDescription->SetText(value);
        songDetailsLoading->get_gameObject()->SetActive(false);
    });

    this->rootModal->Show();
}

