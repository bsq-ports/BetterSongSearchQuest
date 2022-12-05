#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/TableView.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "bsml/shared/BSML.hpp"
#include "assets.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "BeatSaverRegionManager.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/ViewControllers/SongListCell.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))


//////////// Utils

double GetMinStarValue(const SDC_wrapper::BeatStarSong* song) {
    auto diffVec = song->GetDifficultyVector();
    double min = song->GetMaxStarValue();
    for (auto diff: diffVec) {
        if (diff->stars > 0.0 && diff->stars < min) min = diff->stars;
    }
    return min;
}

const std::vector<std::string> CHAR_GROUPING = {"Unknown", "Standard", "OneSaber", "NoArrows", "Lightshow", "NintyDegree", "ThreeSixtyDegree", "Lawless"};
const std::vector<std::string> CHAR_FILTER_OPTIONS = {"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
const std::vector<std::string> DIFFS = {"Easy", "Normal", "Hard", "Expert", "Expert+"};
const std::vector<std::string> REQUIREMENTS = {"Any", "Noodle Extensions", "Mapping Extensions", "Chroma", "Cinema"};

const std::chrono::system_clock::time_point BEATSAVER_EPOCH_TIME_POINT{std::chrono::seconds(FilterOptions::BEATSAVER_EPOCH)};

SortMode prevSort = (SortMode) 0;

using SortFunction = std::function<bool(SDC_wrapper::BeatStarSong const*, SDC_wrapper::BeatStarSong const*)>;

static const std::unordered_map<SortMode, SortFunction> sortFunctionMap = {
        {SortMode::Newest, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Newest
                           {
                               return (struct1->uploaded_unix_time > struct2->uploaded_unix_time);
                           }},
        {SortMode::Oldest, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Oldest
                           {
                               return (struct1->uploaded_unix_time < struct2->uploaded_unix_time);
                           }},
        {SortMode::Latest_Ranked, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Latest Ranked
                           {
                               int struct1RankedUpdateTime;
                               auto struct1DiffVec = struct1->GetDifficultyVector();
                               for(auto const& i : struct1DiffVec)
                               {
                                   struct1RankedUpdateTime = std::max((int)i->ranked_update_time_unix_epoch, struct1RankedUpdateTime);
                               }

                               int struct2RankedUpdateTime;
                               auto struct2DiffVec = struct2->GetDifficultyVector();
                               for(auto const& i : struct2DiffVec)
                               {
                                   struct2RankedUpdateTime = std::max((int)i->ranked_update_time_unix_epoch, struct2RankedUpdateTime);
                               }
                               return (struct1RankedUpdateTime > struct2RankedUpdateTime);
                           }},
        {SortMode::Most_Stars, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Most Stars
                           {
                               return struct1->GetMaxStarValue() > struct2->GetMaxStarValue();
                           }},
        {SortMode::Least_Stars, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Least Stars
                           {
                               return GetMinStarValue(struct1) < GetMinStarValue(struct2);
                           }},
        {SortMode::Best_rated, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Best rated
                           {
                               return (struct1->rating > struct2->rating);
                           }},
        {SortMode::Worst_rated, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Worst rated
                           {
                               return (struct1->rating < struct2->rating);
                           }}
};

////////////

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListController);



void ViewControllers::SongListController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    getLoggerOld().info("Song list contoller activated");
    BSML::parse_and_construct(IncludedAssets::SongList_bsml, this->get_transform(), this);

    sortModeSelections = {StringW("sss")};
    sortModeSelections->Add(StringW("Lol"));
    sortModeSelections->Add(StringW("Cats"));
    selectedSortMode = StringW("Cats");

    if (this->songList != nullptr && this->songList->m_CachedPtr.m_value != nullptr)
    {
        getLoggerOld().info("Table exists");

        songList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);

        
        // limitedUpdateSearchedSongsList = new BetterSongSearch::Util::RatelimitCoroutine([this]()
        //                                                                         { 
        //                                                                             // this->downloadHistoryTable()->ReloadData();
        //                                                                              },
        //                                                                         0.1f);
    }
}

void ViewControllers::SongListController::SelectSong(HMUI::TableView *table, int id)
{
    getLoggerOld().info("Cell clicked %i", id);
}

float ViewControllers::SongListController::CellSize()
{
    return this->cellSize;
}
int ViewControllers::SongListController::NumberOfCells()
{
    return filteredSongList.size();
    return 0;
}

void ViewControllers::SongListController::ctor()
{
    // sortModeSelections = new ListW<StringW>([]);
    INVOKE_CTOR();
    this->cellSize = 8.05f;
}

void ViewControllers::SongListController::SelectRandom() {
    DEBUG("SelectRandom");
};

void ViewControllers::SongListController::ShowMoreModal() {
    DEBUG("ShowMoreModal");
};


void ViewControllers::SongListController::UpdateDataAndFilters () {
    DEBUG("UpdateDataAndFilters");
}

// BSML::CustomCellInfo
HMUI::TableCell *ViewControllers::SongListController::CellForIdx(HMUI::TableView *tableView, int idx)
{
    return ViewControllers::SongListTableData::GetCell(tableView)->PopulateWithSongData(filteredSongList[idx]);
}
