#include "UI/ViewControllers/SongList.hpp"

#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/AudioType.hpp"
#include "UnityEngine/Networking/DownloadHandlerAudioClip.hpp"
#include "UnityEngine/Networking/UnityWebRequestMultimedia.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/InputFieldView.hpp"
#include "HMUI/InputFieldViewChangeBinder.hpp"
#include "HMUI/InputFieldView_InputFieldChanged.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "songloader/shared/API.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/SongPreviewPlayer.hpp"
#include "System/StringComparison.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "bsml/shared/BSML.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "fmt/fmt/include/fmt/core.h"

#include <iterator>
#include <chrono>
#include <string>
#include <algorithm>
#include <functional>
#include <regex>
#include <future>

#include "PluginConfig.hpp"
#include "assets.hpp"
#include "main.hpp"
#include "BeatSaverRegionManager.hpp"
#include "Util/BSMLStuff.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "Util/Random.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/ViewControllers/SongListCell.hpp"
#include "UI/Manager.hpp"
#include "Util/TextUtil.hpp"
#include "Util/BSMLStuff.hpp"

#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))


using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI::Util::BSMLStuff;

#define SONGDOWNLOADER


UnityEngine::GameObject* backButton = nullptr;
UnityEngine::GameObject* soloButton = nullptr;
GlobalNamespace::SongPreviewPlayer* songPreviewPlayer = nullptr;
GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel = nullptr;
GlobalNamespace::LevelCollectionViewController* levelCollectionViewController = nullptr;

DEFINE_TYPE(ViewControllers::SongListController, SongListController);


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

// FIXME: Gives us blank sprite 
UnityEngine::Sprite* GetCoverFromDownloadedSong(const SDC_wrapper::BeatStarSong* song) {
    auto customLevel = RuntimeSongLoader::API::GetLevelByHash(song->hash.string_data);
    if(customLevel.has_value())
        return customLevel.value()->coverImage;
    return nullptr;
}

bool BetterSongSearch::UI::MeetsFilter(const SDC_wrapper::BeatStarSong* song)
{
    auto const& filterOptions = DataHolder::filterOptionsCache;
    std::string songHash = song->hash.string_data;

    if(filterOptions.uploaders.size() != 0) {
		if(std::find(filterOptions.uploaders.begin(), filterOptions.uploaders.end(), removeSpecialCharacter(toLower(song->GetAuthor()))) != filterOptions.uploaders.end()) {
            if(filterOptions.uploadersBlackList)
                return false;
		} else if (!filterOptions.uploadersBlackList) {
			return false;
		}
	}


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

bool BetterSongSearch::UI::DifficultyCheck(const SDC_wrapper::BeatStarSongDifficultyStats* diff, const SDC_wrapper::BeatStarSong* song) {
    auto const& currentFilter = DataHolder::filterOptionsCache;


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
}


void ViewControllers::SongListController::_UpdateSearchedSongsList() {
    DEBUG("_UpdateSearchedSongsList");
    // Skip if not active 
    if (get_isActiveAndEnabled() == false || IsSearching) {
        return;
    }
    
    // Skip if we have no songs
    int totalSongs = DataHolder::songList.size();
    if (totalSongs == 0) {
        return;
    }


    IsSearching = true;
    // Take a snapshot of current filter options
    DataHolder::filterOptionsCache.cache(DataHolder::filterOptions);

    this->searchInProgress->get_gameObject()->set_active(true);

    DEBUG("SortAndFilterSongs runs");
    if(songListTable() != nullptr)
    {
        songListTable()->ClearSelection();
        songListTable()->ClearHighlights();
    }
    // Detect changes
    bool currentFilterChanged = this->filterChanged;
    bool currentSortChanged = prevSort != sort;
    bool currentSearchChanged = prevSearch != search;

    auto currentSort = sort;
    auto currentSearch = search;

    // Reset values
    prevSort = sort;
    prevSearch = search;
    this->filterChanged = false;

    auto searchQuery = split(search, " ");

    bool isRankedSort = sort == SortMode::Most_Stars || sort == SortMode::Least_Stars ||  sort == SortMode::Latest_Ranked;

    std::thread([this, searchQuery, currentSort, currentFilterChanged, currentSortChanged, currentSearchChanged]{
        long long before = 0;
        long long after = 0;
        before = CurrentTimeMs();

        // Prolly need 4 but gotta go fast
        const int num_threads = 8;
        std::thread t[num_threads];
        
      
        // Filter songs if needed
        if (currentFilterChanged) {
            DataHolder::filteredSongList.clear();
            // Set up variables for threads
            int totalSongs = DataHolder::songList.size();

            std::mutex valuesMutex;
            std::atomic_int index = 0;

            //Launch a group of threads
            for (int i = 0; i < num_threads; ++i) {
                t[i] = std::thread([&index, &valuesMutex, totalSongs](){
                    int i = index++;
                    while(i < totalSongs) {
                        auto item = DataHolder::songList[i];
                        bool meetsFilter = MeetsFilter(item);
                        if (meetsFilter) {
                            std::lock_guard<std::mutex> lock(valuesMutex);
                            DataHolder::filteredSongList.push_back(item);
                        }
                        i = index++;
                    }
                });
            }


            //Join the threads with the main thread
            for (int i = 0; i < num_threads; ++i) { t[i].join(); }
        }


        if (currentFilterChanged || currentSearchChanged) {
            DataHolder::searchedSongList.clear();
            // Set up variables for threads
            int totalSongs = DataHolder::filteredSongList.size();
            
            std::mutex valuesMutex;
            std::atomic_int index = 0;

            //Launch a group of threads
            for (int i = 0; i < num_threads; ++i) {
                t[i] = std::thread([&index, &valuesMutex, totalSongs](std::vector<std::string> searchQuery){
                    int i = index++;
                    while(i < totalSongs) {
                        auto item = DataHolder::filteredSongList[i];
                        bool songMeetsSearch = SongMeetsSearch(item, searchQuery);
                        if (songMeetsSearch) {
                            std::lock_guard<std::mutex> lock(valuesMutex);
                            DataHolder::searchedSongList.push_back(item);
                        }
                        i = index++;
                    }
                }, searchQuery);
            }

            //Join the threads with the main thread
            for (int i = 0; i < num_threads; ++i) { t[i].join(); }
        }

        // Sort has to change?
        if (currentFilterChanged || currentSearchChanged || currentSortChanged) {
            std::stable_sort(DataHolder::searchedSongList.begin(), DataHolder::searchedSongList.end(),
                sortFunctionMap.at(currentSort)
            );
        }

        after = CurrentTimeMs();
        DEBUG("Search time: {}ms", after-before);
        QuestUI::MainThreadScheduler::Schedule([this]{
            long long before = 0;
            long long after = 0;
            before = CurrentTimeMs();
            
            // Copy the list to the displayed one
            DataHolder::sortedSongList = DataHolder::searchedSongList;
            this->ResetTable();

            after = CurrentTimeMs();
            INFO("table reset in {} ms",  after-before);

            if(songSearchPlaceholder && songSearchPlaceholder->m_CachedPtr.m_value) {
                if(DataHolder::filteredSongList.size() == DataHolder::songList.size()) {
                    songSearchPlaceholder->set_text("Search by Song, Key, Mapper..");
                } else {
                    songSearchPlaceholder->set_text(fmt::format("Search {} songs", DataHolder::filteredSongList.size()));
                }
            }
            if(!currentSong) {
                SelectSong(songListTable(), 0);
            } else {
                // Always un-select in the list to prevent wrong-selections on resorting, etc.
                songListTable()->ClearSelection();
            }

            this->searchInProgress->get_gameObject()->set_active(false);
            

            // Run search again if something wants to
            bool currentSortChanged = prevSort != sort;
            bool currentSearchChanged = prevSearch != search;
            bool currentFilterChanged = this->filterChanged;

            IsSearching = false;

            // Queue another search at the end of this one
            if (currentSearchChanged || currentSortChanged || currentFilterChanged) {    
                this->UpdateSearchedSongsList();
            }
        });
    }).detach();
}
void ViewControllers::SongListController::UpdateSearchedSongsList() {
    coro(limitedUpdateSearchedSongsList->CallNextFrame());
}

void ViewControllers::SongListController::PostParse() {
    // Steal search box from the base game
    static SafePtrUnity<HMUI::InputFieldView> gameSearchBox;
    if (!gameSearchBox) {
        gameSearchBox = Resources::FindObjectsOfTypeAll<HMUI::InputFieldView *>().First(
        [](HMUI::InputFieldView *x) {
            return x->get_name() == "SearchInputField";
        });
    }

    if (gameSearchBox) {
        // Cleanup the old search
        if (searchBox && searchBox->m_CachedPtr.m_value) {
            DestroyImmediate(searchBox);
        }
        searchBox = Instantiate(gameSearchBox->get_gameObject(), searchBoxContainer->get_transform(), false);
        auto songSearchInput = searchBox->GetComponent<HMUI::InputFieldView *>();
        songSearchPlaceholder = searchBox->get_transform()->Find("PlaceholderText")->GetComponent<HMUI::CurvedTextMeshPro*>();
        songSearchPlaceholder->set_text("Search by Song, Key, Mapper..");
        songSearchInput->keyboardPositionOffset = Vector3(-15, -36, 0);

        std::function<void(HMUI::InputFieldView * view)> onValueChanged = [this](HMUI::InputFieldView * view) {
            DEBUG("Input is: {}", (std::string) view->get_text());
            fcInstance->SongListController->SortAndFilterSongs(this->sort, (std::string) view->get_text() , true);
            // colorPickerModal->Show();
        };
        
        songSearchInput->onValueChanged->AddListener(BSML::MakeUnityAction(onValueChanged));
    }

    // Get the default cover image
    defaultImage = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>().First([](UnityEngine::Sprite* x) {return x->get_name() == "CustomLevelsPack"; });

    // Get song preview player 
    songPreviewPlayer = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SongPreviewPlayer*>().FirstOrDefault();
    levelCollectionViewController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionViewController*>().FirstOrDefault();
    beatmapLevelsModel = QuestUI::ArrayUtil::First(
        UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::BeatmapLevelsModel *>(),
        [](GlobalNamespace::BeatmapLevelsModel *x) {
            return x->customLevelPackCollection != nullptr;
        });

    // BSML has a bug that stops getting the correct platform helper and on game reset it dies and the scrollhelper stays invalid and scroll doesn't work
    auto platformHelper = Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionTableView*>().First()->GetComponentInChildren<HMUI::ScrollView*>()->platformHelper;
    if (platformHelper == nullptr) {
    } else {
        for (auto x: this->GetComponentsInChildren<HMUI::ScrollView*>()){
            x->platformHelper=platformHelper;
        }
    }

    // Make the sort dropdown bigger
    auto c = std::min(9, this->get_sortModeSelections()->size);
    sortDropdown->dropdown->numberOfVisibleCells = c;
    sortDropdown->dropdown->ReloadData();
    auto m = sortDropdown->dropdown->modalView;
    reinterpret_cast<RectTransform *>(m->get_transform())->set_pivot(UnityEngine::Vector2(0.5f, 0.83f + (c * 0.011f)));
}

void ViewControllers::SongListController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    fromBSS = false;

    // Retry if failed to dl
    this->RetryDownloadSongList();

    if (!firstActivation)
        return;

    // Get regional beat saver urls
    BeatSaverRegionManager::RegionLookup();
    
    limitedUpdateSearchedSongsList = new BetterSongSearch::Util::RatelimitCoroutine([this]()
        { 
            this->_UpdateSearchedSongsList();
        }, 0.1f);

    IsSearching = false;
    getLoggerOld().info("Song list contoller activated");

    // Get sort setting from config
    auto sortMode = getPluginConfig().SortMode.GetValue();
    if (sortMode < get_sortModeSelections()->size ) {
        selectedSortMode = get_sortModeSelections()->get_Item(sortMode);
        sort = (SortMode) sortMode;
    }

    BSML::parse_and_construct(IncludedAssets::SongList_bsml, this->get_transform(), this);

    if (this->songList != nullptr && this->songList->m_CachedPtr.m_value != nullptr)
    {
        getLoggerOld().info("Table exists");
        songList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);
    }

    multiDlModal = this->get_gameObject()->AddComponent<UI::Modals::MultiDL*>();
    settingsModal = this->get_gameObject()->AddComponent<UI::Modals::Settings*>();
    uploadDetailsModal = this->get_gameObject()->AddComponent<UI::Modals::UploadDetails*>();

    // Reset table the first time and load data
    if (DataHolder::loadedSDC == true) {
        ViewControllers::SongListController::SortAndFilterSongs(this->sort, "", true);
    } else {
        this->DownloadSongList();
    }

    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/SongList.bsml";
    #endif
}

void ViewControllers::SongListController::SelectSongByHash(std::string hash) {
    for(auto song : DataHolder::songList) {
        if(song->hash.string_data == hash) {
            SetSelectedSong(song);
            return;
        }
    }
    DEBUG("Uh oh, you somehow downloaded a song that was only a figment of your imagination");
}


void ViewControllers::SongListController::SelectSong(HMUI::TableView *table, int id)
{
    if(!table)
        return;
    DEBUG("Cell clicked {}", id);
    if (DataHolder::sortedSongList.size() <= id) {
        // Return if the id is invalid
        return;
    }
    auto song  = DataHolder::sortedSongList[id];
    DEBUG("Selecting song {}", id);
    this->SetSelectedSong(song);
    
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
        DEBUG("Songs size: {}", DataHolder::sortedSongList.size());
        DEBUG("TABLE RESET");
        songListTable()->ReloadData();
        songListTable()->ScrollToCellWithIdx(0, TableView::ScrollPositionType::Beginning, false);
    }
}

int ViewControllers::SongListController::NumberOfCells()
{
    return DataHolder::sortedSongList.size();
}

void ViewControllers::SongListController::ctor()
{
    INVOKE_CTOR();
    selectedSortMode = StringW("Newest");
    this->cellSize = 8.05f;
}

void ViewControllers::SongListController::SelectRandom() {
    DEBUG("SelectRandom");
    auto cellsNumber = this->NumberOfCells();
    if (cellsNumber < 2) {
        return;
    }
    auto id = BetterSongSearch::Util::random(0, cellsNumber - 1);
    songListTable()->SelectCellWithIdx(id, true);
    songListTable()->ScrollToCellWithIdx(id, TableView::ScrollPositionType::Beginning, true);
    
};

void ViewControllers::SongListController::ShowMoreModal() {
    this->moreModal->Show(false, false, nullptr);
    DEBUG("ShowMoreModal");
};
void ViewControllers::SongListController::HideMoreModal() {
    this->moreModal->Hide(false, nullptr);
    DEBUG("HideMoreModal");
};

void ViewControllers::SongListController::ShowCloseConfirmation() {
    this->downloadCancelConfirmModal->Show();
    DEBUG("ShowCloseConfirmation");
};
void ViewControllers::SongListController::ForcedUIClose() {
    fcInstance->ConfirmCancelCallback(true);
    this->downloadCancelConfirmModal->Hide();
};
void ViewControllers::SongListController::ForcedUICloseCancel() {
    fcInstance->ConfirmCancelCallback(false);
    this->downloadCancelConfirmModal->Hide();
};



custom_types::Helpers::Coroutine ViewControllers::SongListController::UpdateDataAndFiltersCoro() {
    // Wait
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.1f));

    // WARNING: There is a bug with bsml update, it runs before the value is changed for some reason
    bool filtersChanged = false;
    

    SortMode sort = prevSort;
    if (selectedSortMode != nullptr) {
        int index = get_sortModeSelections()->IndexOf(selectedSortMode);
        if (index < 0 ) {} else {
            if (index != getPluginConfig().SortMode.GetValue()) {
                filtersChanged = true;
                getPluginConfig().SortMode.SetValue(index);
                sort = (SortMode)index;
            }
        }
    }

    if (filtersChanged) {
        DEBUG("Sort changed");
        this->SortAndFilterSongs(sort, this->prevSearch, true);
    }
}

void ViewControllers::SongListController::UpdateDataAndFilters () {
    coro(UpdateDataAndFiltersCoro());
    DEBUG("UpdateDataAndFilters");
}


void ViewControllers::SongListController::ShowPlaylistCreation() {
    // Hide modal cause bsml does not support automagic hiding of it
    this->moreModal->Hide(false, nullptr);
    DEBUG("ShowPlaylistCreation");
};

void ViewControllers::SongListController::ShowSettings () {
    
    this->settingsModal->OpenModal();
    // Hide modal cause bsml does not support automagic hiding of it
    this->moreModal->Hide(false, nullptr);
    DEBUG("ShowSettings");
}

// Song buttons
void ViewControllers::SongListController::Download () {
    #ifdef SONGDOWNLOADER

    auto songData = this->currentSong;

    if (songData != nullptr) {
        fcInstance->DownloadHistoryViewController->TryAddDownload(songData);
        downloadButton->set_interactable(false);
        // downloadButton->upda;
    }  else {
        WARNING("Current song is null, doing nothing");
    }
    #endif
}

void GetByURLAsync(std::string url, std::function<void(bool success, std::vector<uint8_t>)> finished) {
    BeatSaverRegionManager::GetAsync(url,
           [finished](long httpCode, std::string data) {
            if (httpCode == 200) {
                std::vector<uint8_t> bytes(data.begin(), data.end());
                finished(true, bytes);
            }  else {
                std::vector<uint8_t> bytes;
                finished(false,  bytes);
            }
           }, [](float progress){}
    );
}

custom_types::Helpers::Coroutine GetPreview(std::string url, std::function<void(UnityEngine::AudioClip*)> finished) {
    auto webRequest = UnityEngine::Networking::UnityWebRequestMultimedia::GetAudioClip(url, UnityEngine::AudioType::MPEG);
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(webRequest->SendWebRequest()));
    if(webRequest->get_isNetworkError()) {
        INFO("Network error");
        finished(nullptr);   
        co_return;
    } else {
        while(webRequest->GetDownloadProgress() < 1.0f);
        INFO("Download complete");
        UnityEngine::AudioClip* clip = UnityEngine::Networking::DownloadHandlerAudioClip::GetContent(webRequest);
        finished(clip);
    }
    co_return;
}

custom_types::Helpers::Coroutine EnterSolo(GlobalNamespace::IPreviewBeatmapLevel* level) {
    backButton->GetComponent<UnityEngine::UI::Button *>()->Press();
    co_yield reinterpret_cast<System::Collections::IEnumerator *>(CRASH_UNLESS(UnityEngine::WaitForSeconds::New_ctor(0.5)));
    manager.GoToSongSelect();
    GlobalNamespace::LevelCollectionNavigationController* levelCollectionNavigationController = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionNavigationController*>().FirstOrDefault();
    if(levelCollectionNavigationController) {
        co_yield reinterpret_cast<System::Collections::IEnumerator *>(CRASH_UNLESS(UnityEngine::WaitForSeconds::New_ctor(0.3)));
        levelCollectionNavigationController->SelectLevel(level);
    }
}



void ViewControllers::SongListController::Play () {
    this->PlaySong();
}
void ViewControllers::SongListController::PlaySong (const SDC_wrapper::BeatStarSong* songToPlay) {
    if (songToPlay == nullptr) {
        songToPlay = currentSong;
        if (currentSong == nullptr) {
            return;
        }
    }

    if(fcInstance->ConfirmCancelOfPending([this, songToPlay](){PlaySong(songToPlay);} ))
        return;

    // Make sure to load all downloaded songs before going to the next step
    RuntimeSongLoader::API::RefreshSongs(false, [this, songToPlay](std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&){
        QuestUI::MainThreadScheduler::Schedule(
        [this, songToPlay]
        {
            backButton = UnityEngine::GameObject::Find("BackButton");
            auto level = RuntimeSongLoader::API::GetLevelByHash(std::string(songToPlay->GetHash()));
            if(level.has_value())
            {
                currentLevel = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(level.value());
            }

            fromBSS = true;
            openToCustom = true;
            GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(EnterSolo(currentLevel)));
        });
    });

    
   
}


void ViewControllers::SongListController::ShowBatchDownload () {
    this->multiDlModal->OpenModal();
    this->HideMoreModal();
}

void ViewControllers::SongListController::ShowSongDetails () {
    if (this->currentSong) {
        uploadDetailsModal->OpenModal(this->currentSong);
    }
}

void ViewControllers::SongListController::UpdateDetails () {  
    if (currentSong == nullptr) {
        return;
    }
    
    auto song = currentSong;
    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->GetHash()));
    bool downloaded = beatmap.has_value();
    #ifdef SONGDOWNLOADER
    
    
    // if (downloaded) {
    //     auto cover = GetCoverFromDownloadedSong(song);
    //     this->coverImage->set_sprite(cover);
    // } else {
        this->coverImage->set_sprite(defaultImage);
        std::string newUrl = fmt::format("{}/{}.jpg", BeatSaverRegionManager::coverDownloadUrl, toLower(song->GetHash()));
        DEBUG("{}", newUrl.c_str());
        coverLoading->set_enabled(true);
        GetByURLAsync(newUrl, [this, song](bool success, std::vector<uint8_t> bytes) {
            QuestUI::MainThreadScheduler::Schedule([this, bytes, song, success] {
                if (success) {
                    std::vector<uint8_t> data = bytes;
                    if (song->GetHash() != this->currentSong->GetHash()) return;
                    Array<uint8_t> *spriteArray = il2cpp_utils::vectorToArray(data);
                    this->coverImage->set_sprite(QuestUI::BeatSaberUI::ArrayToSprite(spriteArray));
                    coverLoading->set_enabled(false);
                } else {
                    coverLoading->set_enabled(false);
                }
            });
        });
    // }
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
        if (!getPluginConfig().LoadSongPreviews.GetValue()) {
            
        } else {
            if(!songPreviewPlayer)
                return;

            auto ssp = songPreviewPlayer;
            
            std::string newUrl = fmt::format("{}/{}.mp3", BeatSaverRegionManager::coverDownloadUrl, toLower(song->GetHash()));

            coro(GetPreview(
                newUrl, 
                [ssp](UnityEngine::AudioClip* clip) {
                    if (clip == nullptr) {
                        return;
                    }
                    ssp->CrossfadeTo(clip, -5, 0, clip->get_length(), nullptr);
                }
            ));
        }
        
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

    // downloadButton.set.text = "Download";
    SetIsDownloaded(downloaded);
    selectedSongDiffInfo->set_text(fmt::format("{:.2f} - {:.2f} NPS \n {:.2f} - {:.2f} NJS", minNPS, maxNPS, minNJS, maxNJS));
    selectedSongName->set_text(song->GetName());
    selectedSongAuthor->set_text(song->GetSongAuthor());
}

void ViewControllers::SongListController::FilterByUploader () {
    if (!this->currentSong) {
        return;
    }
    
    fcInstance->FilterViewController->uploadersString = this->currentSong->GetAuthor();
    SetStringSettingValue(fcInstance->FilterViewController->uploadersStringControl, (std::string) this->currentSong->GetAuthor());
    fcInstance->FilterViewController->UpdateFilterSettings();
    DEBUG("FilterByUploader");
}

// BSML::CustomCellInfo
HMUI::TableCell *ViewControllers::SongListController::CellForIdx(HMUI::TableView *tableView, int idx)
{
    return ViewControllers::SongListTableData::GetCell(tableView)->PopulateWithSongData(DataHolder::sortedSongList[idx]);
}

void ViewControllers::SongListController::UpdateSearch() {
    
}

void ViewControllers::SongListController::SortAndFilterSongs(SortMode sort, std::string_view const search, bool resetTable) {
    // Skip if not active 
    if (get_isActiveAndEnabled() == false) {
        return;
    }
    this->sort = sort;
    this->search = search;

    this->UpdateSearchedSongsList();
}

void ViewControllers::SongListController::SetSelectedSong(const SDC_wrapper::BeatStarSong* song) {
    // TODO: Fill all fields, download image, activate buttons
    if (currentSong != nullptr && currentSong->GetHash() == song->GetHash()) {
        return;
    }
    currentSong = song;
    
    DEBUG("Updating details");
    this->UpdateDetails();
}

void ViewControllers::SongListController::SetIsDownloaded(bool isDownloaded,  bool downloadable) {
    playButton->get_gameObject()->set_active(isDownloaded);
    playButton->set_interactable(isDownloaded);
    downloadButton->get_gameObject()->set_active(!isDownloaded);
    downloadButton->set_interactable(!isDownloaded);
    infoButton->set_interactable(true);
}


void ViewControllers::SongListController::DownloadSongList() {
    if (DataHolder::loadingSDC) {
        return;
    }

    fcInstance->FilterViewController->datasetInfoLabel->set_text("Loading dataset...");

    std::thread([this]{
        DataHolder::loadingSDC = true;
        try {
            auto songs = SDC_wrapper::BeatStarSong::GetAllSongs();

            getLoggerOld().info("Successfully got songs!");
            DataHolder::songList = songs;
            
            INFO("Finished loading songs.");
            DataHolder::loadedSDC = true;
            DataHolder::loadingSDC = false;
            DataHolder::failedSDC = false;
            
            if (fcInstance != nullptr && fcInstance->SongListController != nullptr) {
                fcInstance->SongListController->filterChanged = true;
                fcInstance->SongListController->SortAndFilterSongs(fcInstance->SongListController->sort, "", true);
            }
            QuestUI::MainThreadScheduler::Schedule([this]{
                if (this!= nullptr && this->m_CachedPtr.m_value != nullptr && this->get_isActiveAndEnabled() ) {
                    this->filterChanged = true;
                    fcInstance->SongListController->SortAndFilterSongs(this->sort, this->search, true);
                    // filterView.datasetInfoLabel?.SetText($"{songDetails.songs.Length} songs in dataset | Newest: {songDetails.songs.Last().uploadTime.ToLocalTime():d\\. MMM yy - HH:mm}");
                    fcInstance->FilterViewController->datasetInfoLabel->set_text(fmt::format("{} songs in dataset ", DataHolder::songList.size()));
                }
            });
        } catch (...) {
            DataHolder::loadingSDC = false;
            DataHolder::failedSDC = true;
            DataHolder::loadedSDC = false;
            
            QuestUI::MainThreadScheduler::Schedule([this]{
                if (this!= nullptr && this->m_CachedPtr.m_value != nullptr ) {  
                    fcInstance->FilterViewController->datasetInfoLabel->set_text("Failed to load, click to retry");
                }
            });
            getLoggerOld().info("Failed to get songs");
        }
    }).detach();
}

void ViewControllers::SongListController::RetryDownloadSongList() {
    if (DataHolder::failedSDC && !DataHolder::loadingSDC && !DataHolder::loadedSDC) {
        this->DownloadSongList();
    }
}