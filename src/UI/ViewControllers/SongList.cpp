#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"


#include "config-utils/shared/config-utils.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/InputFieldView.hpp"
#include "HMUI/InputFieldViewChangeBinder.hpp"
#include "HMUI/InputFieldView_InputFieldChanged.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/TableView.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/macros.hpp"
#include "assets.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "BeatSaverRegionManager.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "System/StringComparison.hpp"
#include "PluginConfig.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/ViewControllers/SongListCell.hpp"
#include "HMUI/ScrollView.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "Util/BSMLStuff.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "HMUI/TableViewSelectionType.hpp"


using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;


#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))
#include <iomanip>
#include <sstream>
#include <map>
#include <chrono>
#include <string>
#include <algorithm>
#include <functional>
#include <regex>
#include <future>
#include "Util/CurrentTimeMs.hpp"
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

std::string prevSearch;
SortMode prevSort = (SortMode) 0;
int currentSelectedSong = 0;

using SortFunction = std::function<bool(SDC_wrapper::BeatStarSong const*, SDC_wrapper::BeatStarSong const*)>;

//////////////////// UTILS //////////////////////

bool ViewControllers::SongListController::MeetsFilter(const SDC_wrapper::BeatStarSong* song)
{
    auto const& filterOptions = DataHolder::filterOptions;
    std::string songHash = song->hash.string_data;

    /*if(filterOptions->uploaders.size() != 0) {
		if(std::find(filterOptions->uploaders.begin(), filterOptions->uploaders.end(), removeSpecialCharacter(toLower(song->GetAuthor()))) != filterOptions->uploaders.end()) {
		} else {
			return false;
		}
	}*/

    if(song->uploaded_unix_time < filterOptions.minUploadDate)
        return false;

    if(song->GetRating() < filterOptions.minRating) return false;
    if(((int)song->upvotes + (int)song->downvotes) < filterOptions.minVotes) return false;
    bool downloaded = RuntimeSongLoader::API::GetLevelByHash(songHash).has_value();
    if(downloaded)
    {
        if(filterOptions.downloadType == FilterOptions::DownloadFilterType::HideDownloaded)
            return false;
    }
    else
    {
        if(filterOptions.downloadType == FilterOptions::DownloadFilterType::OnlyDownloaded)
            return false;
    }

    bool hasLocalScore = false;

    if(std::find(DataHolder::songsWithScores.begin(), DataHolder::songsWithScores.end(), songHash) != DataHolder::songsWithScores.end())
        hasLocalScore = true;
    //getLogger().info("Checking %s, songs with scores: %u", songHash.c_str(), DataHolder::songsWithScores.size());
    if (hasLocalScore) {
        if(filterOptions.localScoreType == FilterOptions::LocalScoreFilterType::HidePassed)
            return false;
    } else {
        if(filterOptions.localScoreType == FilterOptions::LocalScoreFilterType::OnlyPassed)
            return false;
    }

    bool passesDiffFilter = true;

    for(auto diff : song->GetDifficultyVector())
    {
        if(DifficultyCheck(diff, song)) {
            passesDiffFilter = true;
            break;
        }
        else
            passesDiffFilter = false;
    }

    if(!passesDiffFilter)
        return false;

    if(song->duration_secs < filterOptions.minLength) return false;
    if(song->duration_secs > filterOptions.maxLength) return false;


    return true;
}

bool ViewControllers::SongListController::DifficultyCheck(const SDC_wrapper::BeatStarSongDifficultyStats* diff, const SDC_wrapper::BeatStarSong* song) {
    auto const& currentFilter = DataHolder::filterOptions;

    if(currentFilter.rankedType == FilterOptions::RankedFilterType::OnlyRanked)
        if(!diff->ranked)
            return false;

    if(currentFilter.rankedType != FilterOptions::RankedFilterType::HideRanked)
        if(diff->stars < currentFilter.minStars || diff->stars > currentFilter.maxStars)
            return false;

    switch(currentFilter.difficultyFilter)
    {
        case FilterOptions::DifficultyFilterType::All:
            break;
        case FilterOptions::DifficultyFilterType::Easy:
            if(diff->GetName() != "Easy") return false;
            break;
        case FilterOptions::DifficultyFilterType::Normal:
            if(diff->GetName() != "Normal") return false;
            break;
        case FilterOptions::DifficultyFilterType::Hard:
            if(diff->GetName() != "Hard") return false;
            break;
        case FilterOptions::DifficultyFilterType::Expert:
            if(diff->GetName() != "Expert") return false;
            break;
        case FilterOptions::DifficultyFilterType::ExpertPlus:
            if(diff->GetName() != "ExpertPlus") return false;
            break;
    }

    switch(currentFilter.charFilter) //"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
    {
        case FilterOptions::CharFilterType::All:
            break;
        case FilterOptions::CharFilterType::Custom:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Unknown) return false;
            break;
        case FilterOptions::CharFilterType::Standard:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Standard) return false;
            break;
        case FilterOptions::CharFilterType::OneSaber:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::OneSaber) return false;
            break;
        case FilterOptions::CharFilterType::NoArrows:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::NoArrows) return false;
            break;
        case FilterOptions::CharFilterType::NinetyDegrees:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Degree90) return false;
            break;
        case FilterOptions::CharFilterType::ThreeSixtyDegrees:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Degree360) return false;
            break;
        case FilterOptions::CharFilterType::LightShow:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Lightshow) return false;
            break;
        case FilterOptions::CharFilterType::Lawless:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Lawless) return false;
            break;
    }

    if(diff->njs < currentFilter.minNJS || diff->njs > currentFilter.maxNJS)
        return false;

    auto requirements = diff->GetRequirementVector();
    if(currentFilter.modRequirement != FilterOptions::RequirementType::Any) {
        if(std::find(requirements.begin(), requirements.end(), REQUIREMENTS[(int)currentFilter.modRequirement]) == requirements.end())
            return false;
    }

    if(song->duration_secs > 0) {
        float nps = (float)diff->notes / (float)song->duration_secs;

        if(nps < currentFilter.minNPS || nps > currentFilter.maxNPS)
            return false;
    }

    return true;
}


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

// this hurts
std::vector<std::string> split(std::string_view buffer, const std::string_view delimeter = " ") {
    std::vector<std::string> ret{};
    std::decay_t<decltype(std::string::npos)> pos{};
    while ((pos = buffer.find(delimeter)) != std::string::npos) {
        const auto match = buffer.substr(0, pos);
        if (!match.empty()) ret.emplace_back(match);
        buffer = buffer.substr(pos + delimeter.size());
    }
    if (!buffer.empty()) ret.emplace_back(buffer);
    return ret;
}

std::string removeSpecialCharacter(std::string_view const s) {
    std::string stringy(s);
    for (int i = 0; i < stringy.size(); i++) {

        if (stringy[i] < 'A' || stringy[i] > 'Z' &&
                                stringy[i] < 'a' || stringy[i] > 'z')
        {
            stringy.erase(i, 1);
            i--;
        }
    }
    return stringy;
}

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

bool SongMeetsSearch(const SDC_wrapper::BeatStarSong* song, std::vector<std::string> searchTexts)
{
    int words = 0;
    int matches = 0;

    std::string songName = removeSpecialCharacter(toLower(song->GetName()));
    std::string songSubName = removeSpecialCharacter(toLower(song->GetSubName()));
    std::string songAuthorName = removeSpecialCharacter(toLower(song->GetSongAuthor()));
    std::string levelAuthorName = removeSpecialCharacter(toLower(song->GetAuthor()));
    std::string songKey = toLower(song->key.string_data);

    for (int i = 0; i < searchTexts.size(); i++)
    {
        words++;
        std::string searchTerm = toLower(searchTexts[i]);
        if (i == searchTexts.size() - 1)
        {
            searchTerm.resize(searchTerm.length()-1);
        }


        if (songName.find(searchTerm) != std::string::npos ||
            songSubName.find(searchTerm) != std::string::npos ||
            songAuthorName.find(searchTerm) != std::string::npos ||
            levelAuthorName.find(searchTerm) != std::string::npos ||
            songKey.find(searchTerm) != std::string::npos)
        {
            matches++;
        }
    }

    return matches == words;
    // return true;
}
/////////////////////////////////////////////////

////////////

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListController);



void ViewControllers::SongListController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;
        
    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/SongList.bsml";
    #endif
    getLoggerOld().info("Song list contoller activated");
    BSML::parse_and_construct(IncludedAssets::SongList_bsml, this->get_transform(), this);

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

    // Steal search box from the base game
    static SafePtrUnity<HMUI::InputFieldView> searchBox;
    if (!searchBox) {
        searchBox = Resources::FindObjectsOfTypeAll<HMUI::InputFieldView *>().First(
        [](HMUI::InputFieldView *x) {
            return x->get_name() == "SearchInputField";
        });
    }

    if (searchBox) {
        auto newSearchBox = Instantiate(searchBox->get_gameObject(), searchBoxContainer->get_transform(), false);
        auto songSearchInput = newSearchBox->GetComponent<HMUI::InputFieldView *>();
        auto songSearchPlaceholder = newSearchBox->get_transform()->Find("PlaceholderText")->GetComponent<HMUI::CurvedTextMeshPro*>();

        songSearchInput->keyboardPositionOffset = Vector3(-15, -36, 0);

        std::function<void(HMUI::InputFieldView * view)> onClick = [this](HMUI::InputFieldView * view) {
            DEBUG("Input is: {}", (std::string) view->get_text());
            fcInstance->SongListController->SortAndFilterSongs(SortMode::Newest, (std::string) view->get_text() , true);
            // colorPickerModal->Show();
        };
        
        songSearchInput->onValueChanged->AddListener(BSML::MakeUnityAction(onClick));
    }

    
    auto ivrhelper = BSML::Helpers::GetIVRPlatformHelper();
    for (auto x: this->GetComponentsInChildren<HMUI::ScrollView*>()){
        x->platformHelper=ivrhelper;
    }


    std::function<void(HMUI::TableView *table, int id)> ss = [this](HMUI::TableView *table, int id) {
        DEBUG("EVENT FIRED, YAY {}", id);
    };
    songListTable()->add_didSelectCellWithIdxEvent(BSML::MakeSystemAction(ss));

    // Util::BSMLStuff::GetScrollbarForTable(songListTable()->get_gameObject(),  scrollBarContainer->get_transform());

    // Reset table the first time and load data
    ViewControllers::SongListController::ResetTable();

    
}

void ViewControllers::SongListController::SelectSong(HMUI::TableView *table, int id)
{
    
    DEBUG("Cell clicked {}", id);
}

float ViewControllers::SongListController::CellSize()
{
    // TODO: Different font sizes
    return true ? 11.66f : 14.0f;
}

void ViewControllers::SongListController::ResetTable()
{

    if(songListTable() != nullptr)
    {
        DEBUG("Songs size: {}", DataHolder::filteredSongList.size());
        DEBUG("TABLE RESET");
        songListTable()->ReloadData();
        songListTable()->ScrollToCellWithIdx(0, TableView::ScrollPositionType::Beginning, false);
    }
}

int ViewControllers::SongListController::NumberOfCells()
{
    return DataHolder::filteredSongList.size();
}

void ViewControllers::SongListController::ctor()
{
    INVOKE_CTOR();
    selectedSortMode = StringW("Newest");
    this->cellSize = 8.05f;
}

void ViewControllers::SongListController::SelectRandom() {
    DEBUG("SelectRandom");
    // TODO: Remove. It's here for testing
    fcInstance->SongListController->SortAndFilterSongs(SortMode::Newest, "", true);
};

void ViewControllers::SongListController::ShowMoreModal() {
    this->moreModal->Show(false, false, nullptr);
    DEBUG("ShowMoreModal");
};
void ViewControllers::SongListController::HideMoreModal() {
    this->moreModal->Hide(false, nullptr);
    DEBUG("HideMoreModal");
};


void ViewControllers::SongListController::UpdateDataAndFilters () {
    DEBUG("UpdateDataAndFilters");
}

void ViewControllers::SongListController::ForcedUIClose() {
    DEBUG("ForcedUIClose");
};

void ViewControllers::SongListController::ForcedUICloseCancel () {
    DEBUG("ForcedUICloseCancel");
}

void ViewControllers::SongListController::ShowPlaylistCreation() {
    // Hide modal cause bsml does not support automagic hiding of it
    this->moreModal->Hide(false, nullptr);
    DEBUG("ShowPlaylistCreation");
};

void ViewControllers::SongListController::ShowSettings () {
    // Hide modal cause bsml does not support automagic hiding of it
    this->moreModal->Hide(false, nullptr);
    DEBUG("ShowSettings");
}

// Song buttons
void ViewControllers::SongListController::Download () {
    DEBUG("Download");
}

void ViewControllers::SongListController::Play () {
    DEBUG("Play");
}

void ViewControllers::SongListController::ShowSongDetails () {  
    DEBUG("ShowSongDetails");
}

void ViewControllers::SongListController::FilterByUploader () {
  
    DEBUG("FilterByUploader");
}

// BSML::CustomCellInfo
HMUI::TableCell *ViewControllers::SongListController::CellForIdx(HMUI::TableView *tableView, int idx)
{
    return ViewControllers::SongListTableData::GetCell(tableView)->PopulateWithSongData(DataHolder::filteredSongList[idx]);
}


void ViewControllers::SongListController::SortAndFilterSongs(SortMode sort, std::string_view const search, bool resetTable) {
    DEBUG("SortAndFilterSongs");
    if(songListTable() != nullptr)
    {
        songListTable()->ClearSelection();
        songListTable()->ClearHighlights();
        this->searchInProgress->get_gameObject()->set_active(false);
    }
    prevSort = sort;
    prevSearch = search;
    // std::thread([&]{
        bool isRankedSort = sort == SortMode::Most_Stars || sort == SortMode::Least_Stars ||  sort == SortMode::Latest_Ranked;
        

        long long before = 0;
        long long after = 0;

    // /// Original
    // before = CurrentTimeMs();
    // DataHolder::filteredSongList.clear();
    // for(auto song : DataHolder::songList)
    // {
    //     bool meetsFilter = MeetsFilter(song);
    //     bool songMeetsSearch = SongMeetsSearch(song, split(search, " "));
    //     if (meetsFilter) {
            
    //         if (songMeetsSearch) {
    //             DataHolder::filteredSongList.emplace_back(song);
    //         }     
    //     }
    // }
    // after = CurrentTimeMs();
    // DEBUG("Time before: {}", after-before);

    // // Time the sort
    // before = CurrentTimeMs();
    // std::stable_sort(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end(),
    //     sortFunctionMap.at(sort)
    // );
    // after = CurrentTimeMs();
    // DEBUG("Time sorting: {}", after-before);

    // /// Barely optimized
    // before = CurrentTimeMs();
    // DataHolder::filteredSongList.clear();
    // for(auto song : DataHolder::songList)
    // {
    //     bool meetsFilter = MeetsFilter(song);
    //     if (meetsFilter) {
    //         bool songMeetsSearch = SongMeetsSearch(song, split(search, " "));
    //         if (songMeetsSearch) {
    //             DataHolder::filteredSongList.emplace_back(song);
    //         }     
    //     }
    // }
    // after = CurrentTimeMs();
    // DEBUG("Time new: {}", after-before);


    // before = CurrentTimeMs();
    // DataHolder::filteredSongList.clear();
    // for(auto song : DataHolder::songList)
    // {
    //     bool meetsFilter = MeetsFilter(song);
    //     if (meetsFilter) {
    //         // bool songMeetsSearch = SongMeetsSearch(song, split(search, " "));
    //         // if (songMeetsSearch) {
    //         //     DataHolder::filteredSongList.emplace_back(song);
    //         // }     
    //         DataHolder::filteredSongList.emplace_back(song);
    //     }
    // }
    // after = CurrentTimeMs();
    // DEBUG("Time filter: {}", after-before);


    // before = CurrentTimeMs();
    // DataHolder::filteredSongList.clear();
    // for(auto song : DataHolder::songList)
    // {
    //     bool songMeetsSearch = SongMeetsSearch(song, split(search, " "));
    //     if (songMeetsSearch) {
    //         DataHolder::filteredSongList.emplace_back(song);
    //     }     
    // }
    // after = CurrentTimeMs();
    // DEBUG("Time search: {}", after-before);


    auto searchQuery = split(search, " ");
    std::thread([this, searchQuery, sort]{
        long long before = 0;
        long long after = 0;
        before = CurrentTimeMs();
        DataHolder::tempSongList.clear();
        for(auto song : DataHolder::songList)
        {
            bool meetsFilter = MeetsFilter(song);
            if (meetsFilter) {
                bool songMeetsSearch = SongMeetsSearch(song, searchQuery);
                if (songMeetsSearch) {
                    DataHolder::tempSongList.emplace_back(song);
                }
            }
        }

        std::stable_sort(DataHolder::tempSongList.begin(), DataHolder::tempSongList.end(),
            sortFunctionMap.at(sort)
        );

        after = CurrentTimeMs();
        DEBUG("Time searchOffthread: {}", after-before);
        QuestUI::MainThreadScheduler::Schedule([this]{
            long long before = 0;
            long long after = 0;
            before = CurrentTimeMs();

            DataHolder::filteredSongList  = DataHolder::tempSongList;
            this->ResetTable();



            after = CurrentTimeMs();
            // Three dls to test selection
            if (DataHolder::filteredSongList.size() > 22) {
                // Add 20 songs to test the other table
                for (int i= 0; i<20;i++ ) {
                    fcInstance->DownloadHistoryViewController->TryAddDownload(DataHolder::filteredSongList[i]);
                }
            }   
            INFO("table reset in {}ms",  after-before);
        });
    }).detach();
}

// Old bench results in ms
// Time before: 3526
// Time sorting: 251
// Time new: 3449
// Time filter: 2659
// Time search: 797