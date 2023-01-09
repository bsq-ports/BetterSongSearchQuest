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
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "System/StringComparison.hpp"
#include "System/Nullable_1.hpp"
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
#include "System/Threading/Tasks/Task.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "Util/Debug.hpp"
#include <cmath>
#include "song-details/shared/SongDetails.hpp"

#define coro(coroutine) SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))


using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI::Util::BSMLStuff;
using namespace GlobalNamespace;

#define SONGDOWNLOADER


UnityEngine::GameObject* backButton = nullptr;
UnityEngine::GameObject* soloButton = nullptr;
SongPreviewPlayer* songPreviewPlayer = nullptr;
BeatmapLevelsModel* beatmapLevelsModel = nullptr;
LevelCollectionViewController* levelCollectionViewController = nullptr;

DEFINE_TYPE(ViewControllers::SongListController, SongListController);


//////////// Utils



const std::vector<std::string> CHAR_GROUPING = {"Unknown", "Standard", "OneSaber", "NoArrows", "Lightshow", "NintyDegree", "ThreeSixtyDegree", "Lawless"};
const std::vector<std::string> CHAR_FILTER_OPTIONS = {"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
const std::vector<std::string> DIFFS = {"Easy", "Normal", "Hard", "Expert", "Expert+"};
const std::vector<std::string> REQUIREMENTS = {"Any", "Noodle Extensions", "Mapping Extensions", "Chroma", "Cinema"};

const std::chrono::system_clock::time_point BEATSAVER_EPOCH_TIME_POINT{std::chrono::seconds(FilterOptions::BEATSAVER_EPOCH)};


std::string prevSearch;
SortMode prevSort = (SortMode) 0;
int currentSelectedSong = 0;

using SortFunction = std::function<bool(SongDetailsCache::Song const* , SongDetailsCache::Song const*)>;

//////////////////// UTILS //////////////////////
bool BetterSongSearch::UI::MeetsFilter(const SongDetailsCache::Song* song)
{
    auto const& filterOptions = DataHolder::filterOptionsCache;
    std::string songHash = song->hash();

    if(filterOptions.uploaders.size() != 0) {
		if(std::find(filterOptions.uploaders.begin(), filterOptions.uploaders.end(), removeSpecialCharacter(toLower(song->levelAuthorName()))) != filterOptions.uploaders.end()) {
            if(filterOptions.uploadersBlackList)
                return false;
		} else if (!filterOptions.uploadersBlackList) {
			return false;
		}
	}


    if(song->uploadTimeUnix < filterOptions.minUploadDate)
        return false;

    float songRating = std::isnan(song->rating())? 0: song->rating();
    if(songRating < filterOptions.minRating) return false;
    
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

    for(const auto& diff : *song)
    {
        if(DifficultyCheck(&diff, song)) {
            passesDiffFilter = true;
            break;
        }
        else
            passesDiffFilter = false;
    }

    if(!passesDiffFilter)
        return false;

    if(song->songDurationSeconds < filterOptions.minLength) return false;
    if(song->songDurationSeconds > filterOptions.maxLength) return false;


    return true;
}

bool BetterSongSearch::UI::DifficultyCheck(const SongDetailsCache::SongDifficulty* diff, const SongDetailsCache::Song* song) {
    auto const& currentFilter = DataHolder::filterOptionsCache;


    if(currentFilter.rankedType == FilterOptions::RankedFilterType::OnlyRanked)
        if(!diff->ranked())
            return false;

    // If we have a ranked sort, we force ranked filter
    if (currentFilter.isRankedSort) {
        if (!diff->ranked())
            return false;
    }

    if(currentFilter.rankedType != FilterOptions::RankedFilterType::HideRanked)
        if(diff->stars < currentFilter.minStars || diff->stars > currentFilter.maxStars)
            return false;

    switch(currentFilter.difficultyFilter)
    {
        case FilterOptions::DifficultyFilterType::All:
            break;
        case FilterOptions::DifficultyFilterType::Easy:
            if(diff->difficulty != SongDetailsCache::MapDifficulty::Easy) return false;
            break;
        case FilterOptions::DifficultyFilterType::Normal:
            if(diff->difficulty != SongDetailsCache::MapDifficulty::Normal) return false;
            break;
        case FilterOptions::DifficultyFilterType::Hard:
            if(diff->difficulty != SongDetailsCache::MapDifficulty::Hard) return false;
            break;
        case FilterOptions::DifficultyFilterType::Expert:
            if(diff->difficulty != SongDetailsCache::MapDifficulty::Expert) return false;
            break;
        case FilterOptions::DifficultyFilterType::ExpertPlus:
            if(diff->difficulty != SongDetailsCache::MapDifficulty::ExpertPlus) return false;
            break;
    }

    switch(currentFilter.charFilter) //"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
    {
        case FilterOptions::CharFilterType::All:
            break;
        case FilterOptions::CharFilterType::Custom:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::Custom) return false;
            break;
        case FilterOptions::CharFilterType::Standard:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::Standard) return false;
            break;
        case FilterOptions::CharFilterType::OneSaber:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::OneSaber) return false;
            break;
        case FilterOptions::CharFilterType::NoArrows:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::NoArrows) return false;
            break;
        case FilterOptions::CharFilterType::NinetyDegrees:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::NinetyDegree) return false;
            break;
        case FilterOptions::CharFilterType::ThreeSixtyDegrees:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::ThreeSixtyDegree) return false;
            break;
        case FilterOptions::CharFilterType::LightShow:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::LightShow) return false;
            break;
        case FilterOptions::CharFilterType::Lawless:
            if(diff->characteristic != SongDetailsCache::MapCharacteristic::Lawless) return false;
            break;
    }

    if(diff->njs < currentFilter.minNJS || diff->njs > currentFilter.maxNJS)
        return false;

    // auto requirements = diff->mods;
    // if(currentFilter.modRequirement != FilterOptions::RequirementType::Any) {
    //     [(int)currentFilter.modRequirement])
    //     if () {

    //     }
    //     if(std::find(requirements.begin(), requirements.end(), REQUIREMENTS[(int)currentFilter.modRequirement]) == requirements.end())
    //         return false;
    // }

    if(song->songDurationSeconds > 0) {
        float nps = (float)diff->notes / (float)song->songDurationSeconds;

        if(nps < currentFilter.minNPS || nps > currentFilter.maxNPS)
            return false;
    }

    return true;
}


static const std::unordered_map<SortMode, SortFunction> sortFunctionMap = {
        {SortMode::Newest, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Newest
                           {
                               return (struct1->uploadTimeUnix > struct2->uploadTimeUnix);
                           }},
        {SortMode::Oldest, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Oldest
                           {
                               return (struct1->uploadTimeUnix < struct2->uploadTimeUnix);
                           }},
        {SortMode::Latest_Ranked, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Latest Ranked
            {
         
                return (struct1->rankedChangeUnix > struct2->rankedChangeUnix);
            }},
        {SortMode::Most_Stars, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Most Stars
                           {
                               return struct1->maxStar() > struct2->maxStar();
                           }},
        {SortMode::Least_Stars, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Least Stars
                           {
                               return struct1->minStar() < struct2->minStar();
                           }},
        {SortMode::Best_rated, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Best rated
                           {
                                // Move nan to the end
                               float v1 = std::isnan(struct1->rating())? 0: struct1->rating();
                               float v2 = std::isnan(struct2->rating())? 0: struct2->rating();
                               return (v1 > v2);
                           }},
        {SortMode::Worst_rated, [] (const SongDetailsCache::Song* struct1, const SongDetailsCache::Song* struct2)//Worst rated
                            {
                                // Move nan to the end
                                float v1 = std::isnan(struct1->rating())? 9999: struct1->rating();
                                float v2 = std::isnan(struct2->rating())? 9999: struct2->rating();
                                return (v1 < v2);
                           }}
};


bool SongMeetsSearch(const SongDetailsCache::Song* song, std::vector<std::string> searchTexts)
{
    int words = 0;
    int matches = 0;

    std::string songName = removeSpecialCharacter(toLower(song->songName()));
    // std::string songSubName = removeSpecialCharacter(toLower(song->son));
    std::string songAuthorName = removeSpecialCharacter(toLower(song->songAuthorName()));
    std::string levelAuthorName = removeSpecialCharacter(toLower(song->levelAuthorName()));
    std::string songKey = toLower(song->key());

    for (int i = 0; i < searchTexts.size(); i++)
    {
        words++;
        std::string searchTerm = toLower(searchTexts[i]);
        if (i == searchTexts.size() - 1)
        {
            searchTerm.resize(searchTerm.length()-1);
        }


        if (songName.find(searchTerm) != std::string::npos ||
            // songSubName.find(searchTerm) != std::string::npos ||
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
    
    // Skip if song details is null
    if (DataHolder::songDetails == nullptr) {
        return;
    }

    // Skip if we have no songs
    int totalSongs = DataHolder::songDetails->songs.size();
    if (totalSongs == 0) {
        return;
    }


    IsSearching = true;

    // Check if sort is ranked and set the filter
    bool isRankedSort = sort == SortMode::Most_Stars || sort == SortMode::Least_Stars ||  sort == SortMode::Latest_Ranked;
    DataHolder::filterOptions.isRankedSort = isRankedSort;
    // Detect changes
    bool currentFilterChanged = this->filterChanged;
    bool currentSortChanged = prevSort != sort;
    bool currentSearchChanged = prevSearch != search;
    bool rankedSortChanged = DataHolder::filterOptionsCache.isRankedSort != isRankedSort;

    // Take a snapshot of current filter options
    DataHolder::filterOptionsCache.cache(DataHolder::filterOptions);

    DEBUG("SEARCHING Cache");
    DEBUG("Sort: {}", SortToString((int) sort));
    DEBUG("Requirement: {}", RequirementTypeToString((int) DataHolder::filterOptionsCache.modRequirement));
    DEBUG("Char: {}", CharFilterTypeToString((int) DataHolder::filterOptionsCache.charFilter));
    DEBUG("Difficulty: {}", DifficultyFilterTypeToString((int) DataHolder::filterOptionsCache.difficultyFilter));
    DEBUG("Ranked: {}", RankedFilterTypeToString((int) DataHolder::filterOptionsCache.rankedType));
    DEBUG("LocalScore: {}", LocalScoreFilterTypeToString((int) DataHolder::filterOptionsCache.localScoreType));
    DEBUG("Download: {}", DownloadFilterTypeToString((int) DataHolder::filterOptionsCache.downloadType));
    DEBUG("MinNJS: {}", DataHolder::filterOptionsCache.minNJS);
    DEBUG("MaxNJS: {}", DataHolder::filterOptionsCache.maxNJS);
    DEBUG("MinNPS: {}", DataHolder::filterOptionsCache.minNPS);
    DEBUG("MaxNPS: {}", DataHolder::filterOptionsCache.maxNPS);
    DEBUG("MinStars: {}", DataHolder::filterOptionsCache.minStars);
    DEBUG("MaxStars: {}", DataHolder::filterOptionsCache.maxStars);
    DEBUG("MinLength: {}", DataHolder::filterOptionsCache.minLength);
    DEBUG("MaxLength: {}", DataHolder::filterOptionsCache.maxLength);
    DEBUG("MinRating: {}", DataHolder::filterOptionsCache.minRating);
    DEBUG("MinVotes: {}",  DataHolder::filterOptionsCache.minVotes);
    DEBUG("Uploaders number: {}",  DataHolder::filterOptionsCache.uploaders.size());
    DEBUG("IsRankedSort: {}", DataHolder::filterOptionsCache.isRankedSort);
    DEBUG("RankedSortChanged: {}", rankedSortChanged);

    this->searchInProgress->get_gameObject()->set_active(true);

    DEBUG("SortAndFilterSongs runs");
    if(songListTable() != nullptr)
    {
        songListTable()->ClearSelection();
        songListTable()->ClearHighlights();
    }
 

    // Grab current values for sort and search
    auto currentSort = sort;
    auto currentSearch = search;

    // Reset values
    prevSort = sort;
    prevSearch = search;
    this->filterChanged = false;

    auto searchQuery = split(currentSearch, " ");


    std::thread([this, searchQuery, currentSort, currentFilterChanged, currentSortChanged, currentSearchChanged, rankedSortChanged]{
        long long before = 0;
        long long after = 0;
        before = CurrentTimeMs();

        // Prolly need 4 but gotta go fast
        const int num_threads = 8;
        std::thread t[num_threads];
        
      
        // Filter songs if needed
        if (currentFilterChanged || rankedSortChanged) {
            DEBUG("Filtering");
            DataHolder::filteredSongList.clear();
            // Set up variables for threads
            int totalSongs = DataHolder::songDetails->songs.size();

            std::mutex valuesMutex;
            std::atomic_int index = 0;

            //Launch a group of threads
            for (int i = 0; i < num_threads; ++i) {
                t[i] = std::thread([&index, &valuesMutex, totalSongs](){
                    int i = index++;
                    while(i < totalSongs) {
                        const SongDetailsCache::Song & item = DataHolder::songDetails->songs.at(i);
                        bool meetsFilter = MeetsFilter(&item);
                        if (meetsFilter) {
                            std::lock_guard<std::mutex> lock(valuesMutex);
                            DataHolder::filteredSongList.push_back(&item);
                        }
                        i = index++;
                    }
                });
            }


            //Join the threads with the main thread
            for (int i = 0; i < num_threads; ++i) { t[i].join(); }
        }


        if (currentFilterChanged || rankedSortChanged || currentSearchChanged) {
            DEBUG("Searching");
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
        if (currentFilterChanged  || rankedSortChanged || currentSearchChanged || currentSortChanged) {
            DEBUG("Sorting");
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
            DataHolder::sortedSongList.clear();
            DataHolder::sortedSongList = DataHolder::searchedSongList;
            this->ResetTable();

            after = CurrentTimeMs();
            INFO("table reset in {} ms",  after-before);

            if(songSearchPlaceholder && songSearchPlaceholder->m_CachedPtr.m_value) {
                if(DataHolder::filteredSongList.size() == DataHolder::songDetails->songs.size()) {
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
            this->SortAndFilterSongs(this->sort, (std::string) view->get_text(), true);
        };
        
        songSearchInput->onValueChanged->AddListener(BSML::MakeUnityAction(onValueChanged));
    }

    // Get the default cover image
    defaultImage = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>().First([](UnityEngine::Sprite* x) {return x->get_name() == "CustomLevelsPack"; });

    // Get song preview player 
    songPreviewPlayer = UnityEngine::Resources::FindObjectsOfTypeAll<SongPreviewPlayer*>().FirstOrDefault();
    levelCollectionViewController = UnityEngine::Resources::FindObjectsOfTypeAll<LevelCollectionViewController*>().FirstOrDefault();
    beatmapLevelsModel = QuestUI::ArrayUtil::First(
        UnityEngine::Resources::FindObjectsOfTypeAll<BeatmapLevelsModel *>(),
        [](BeatmapLevelsModel *x) {
            return x->customLevelPackCollection != nullptr;
        });

    // BSML has a bug that stops getting the correct platform helper and on game reset it dies and the scrollhelper stays invalid and scroll doesn't work
    auto platformHelper = Resources::FindObjectsOfTypeAll<LevelCollectionTableView*>().First()->GetComponentInChildren<HMUI::ScrollView*>()->platformHelper;
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

    // Get coordinators
    soloFreePlayFlowCoordinator = UnityEngine::Object::FindObjectOfType<SoloFreePlayFlowCoordinator*>();
    multiplayerLevelSelectionFlowCoordinator = UnityEngine::Object::FindObjectOfType<MultiplayerLevelSelectionFlowCoordinator*>();

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

        // Reinitialize the label to show number of songs
        if (fcInstance->FilterViewController != nullptr && fcInstance->FilterViewController->m_CachedPtr.m_value != nullptr) {
            fcInstance->FilterViewController->datasetInfoLabel->set_text(fmt::format("{} songs in dataset ", DataHolder::songDetails->songs.size()));
        }
    } else {
        this->DownloadSongList();
    }
    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/SongList.bsml";
    #endif
}

void ViewControllers::SongListController::SelectSongByHash(std::string hash) {
    if (DataHolder::songDetails == nullptr) {
        DEBUG("Song details is not loaded yet");
        return;
    }

    DEBUG("Song hash: {}", hash);
    auto & song = DataHolder::songDetails->songs.FindByHash(hash);
    if (song == SongDetailsCache::Song::none) {
        DEBUG("Uh oh, you somehow downloaded a song that was only a figment of your imagination");
        return;
    }

    SetSelectedSong(&song);
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
    bool smallFont = getPluginConfig().SmallerFontSize.GetValue();
    return smallFont ? 11.66f : 14.0f;
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
    songListTable()->ScrollToCellWithIdx(id, TableView::ScrollPositionType::Beginning, false);
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
        DEBUG("Clip size: {}", pretty_bytes(webRequest->get_downloadedBytes()));
        finished(clip);
    }
    co_return;
}

void ViewControllers::SongListController::EnterSolo(IPreviewBeatmapLevel* level) {
    fcInstance->Close(true, false);
    
    auto customLevelsPack = RuntimeSongLoader::API::GetCustomLevelsPack();
    auto category = SelectLevelCategoryViewController::LevelCategory(SelectLevelCategoryViewController::LevelCategory::All);
    
    auto state = LevelSelectionFlowCoordinator::State::New_ctor(
        System::Nullable_1(category, true),
        (IBeatmapLevelPack*) customLevelsPack->CustomLevelsPack ,
        level,
        nullptr
    );
    multiplayerLevelSelectionFlowCoordinator->LevelSelectionFlowCoordinator::Setup(state);
    soloFreePlayFlowCoordinator->Setup(state);

    manager.GoToSongSelect();
}



void ViewControllers::SongListController::Play () {
    this->PlaySong();
}
void ViewControllers::SongListController::PlaySong (const SongDetailsCache::Song* songToPlay) {
    if (songToPlay == nullptr) {
        songToPlay = currentSong;
        if (currentSong == nullptr) {
            return;
        }
    }

    if(fcInstance->ConfirmCancelOfPending([this, songToPlay](){PlaySong(songToPlay);} ))
        return;

    // Hopefully not leaking any memory
    auto fun = [this, songToPlay](){
        QuestUI::MainThreadScheduler::Schedule(
        [this, songToPlay]
        {
            auto level = RuntimeSongLoader::API::GetLevelByHash(std::string(songToPlay->hash()));
            if(level.has_value())
            {
                currentLevel = reinterpret_cast<IPreviewBeatmapLevel*>(level.value());
            } else {
                ERROR("Song somehow is not downloaded, pls fix");
                return;
            }

            fromBSS = true;
            openToCustom = true;
            EnterSolo(currentLevel);
        });
    };

    if (fcInstance->DownloadHistoryViewController->hasUnloadedDownloads) {
        RuntimeSongLoader::API::RefreshSongs(false, 
        [fun](std::vector<CustomPreviewBeatmapLevel*> const&){
            fun();
        });

    } else {
        fun();
    }
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
    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->hash()));
    bool loaded = beatmap.has_value();
    bool downloaded = fcInstance->DownloadHistoryViewController->CheckIsDownloaded(std::string(song->hash()));
    #ifdef SONGDOWNLOADER
    
    if (songAssetLoadCanceller != nullptr) {
        songAssetLoadCanceller->Cancel();
    }
	songAssetLoadCanceller = System::Threading::CancellationTokenSource::New_ctor();
    // if beatmap is loaded
    if (loaded) {
        using Task = System::Threading::Tasks::Task_1<UnityEngine::Sprite*>*;
        using Action = System::Action_1<System::Threading::Tasks::Task*>*;

        Task coverTask = beatmap.value()->GetCoverImageAsync(songAssetLoadCanceller->get_Token());     
        auto action = custom_types::MakeDelegate<Action>(classof(Action), static_cast<std::function<void(Task)>>([this](Task resultTask) {
            UnityEngine::Sprite* cover = resultTask->get_ResultOnSuccess();
            if (cover) {
                this->coverImage->set_sprite(cover);
                coverLoading->set_enabled(false);
            }
        }));
        reinterpret_cast<System::Threading::Tasks::Task*>(coverTask)->ContinueWith(action);
    } else {
        
        std::string newUrl = fmt::format("{}/{}.jpg", BeatSaverRegionManager::coverDownloadUrl, toLower(song->hash()));
        DEBUG("{}", newUrl.c_str());
        coverLoading->set_enabled(true);
        GetByURLAsync(newUrl, [this, song](bool success, std::vector<uint8_t> bytes) {
            QuestUI::MainThreadScheduler::Schedule([this, bytes, song, success] {
                if (success) {
                    std::vector<uint8_t> data = bytes;
                    DEBUG("Image size: {}", pretty_bytes(bytes.size()));
                    if (song->hash() != this->currentSong->hash()) return;
                    Array<uint8_t> *spriteArray = il2cpp_utils::vectorToArray(data);
                    this->coverImage->set_sprite(QuestUI::BeatSaberUI::ArrayToSprite(spriteArray));
                    coverLoading->set_enabled(false);
                } else {
                    coverLoading->set_enabled(false);
                }
            });
        });
    }

    // If the song is loaded then get it from local sources
    if(loaded) {
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
            
            std::string newUrl = fmt::format("{}/{}.mp3", BeatSaverRegionManager::coverDownloadUrl, toLower(song->hash()));

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
    for (const auto &diff: *song) {
        float nps = (float) diff.notes / (float) song->songDurationSeconds;
        float njs = diff.njs;
        minNPS = std::min(nps, minNPS);
        maxNPS = std::max(nps, maxNPS);

        minNJS = std::min(njs, minNJS);
        maxNJS = std::max(njs, maxNJS);
    }

    // downloadButton.set.text = "Download";
    SetIsDownloaded(downloaded);
    selectedSongDiffInfo->set_text(fmt::format("{:.2f} - {:.2f} NPS \n {:.2f} - {:.2f} NJS", minNPS, maxNPS, minNJS, maxNJS));
    selectedSongName->set_text(song->songName());
    selectedSongAuthor->set_text(song->songAuthorName());
}

void ViewControllers::SongListController::FilterByUploader () {
    if (!this->currentSong) {
        return;
    }
    
    fcInstance->FilterViewController->uploadersString = this->currentSong->levelAuthorName();
    SetStringSettingValue(fcInstance->FilterViewController->uploadersStringControl, (std::string) this->currentSong->levelAuthorName());
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

void ViewControllers::SongListController::SetSelectedSong(const SongDetailsCache::Song* song) {
    // TODO: Fill all fields, download image, activate buttons
    if (currentSong != nullptr && currentSong->hash() == song->hash()) {
        return;
    }
    currentSong = song;

    LogSongInfo(song);

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
            auto songDetails = SongDetailsCache::SongDetails::Init().get();
            DataHolder::songDetails = songDetails;
            INFO("Finished loading songs.");
            DataHolder::loadedSDC = true;
            DataHolder::loadingSDC = false;
            DataHolder::failedSDC = false;
            
            // Initial search
            if (fcInstance != nullptr && fcInstance->SongListController != nullptr) {
                fcInstance->SongListController->filterChanged = true;
                fcInstance->SongListController->SortAndFilterSongs(fcInstance->SongListController->sort, "", true);
            }
            QuestUI::MainThreadScheduler::Schedule([this, songDetails]{
                if (this!= nullptr && this->m_CachedPtr.m_value != nullptr && this->get_isActiveAndEnabled() ) {
                    this->filterChanged = true;
                    fcInstance->SongListController->SortAndFilterSongs(this->sort, this->search, true);
                    // filterView.datasetInfoLabel?.SetText($"{songDetails.songs.Length} songs in dataset | Newest: {songDetails.songs.Last().uploadTime.ToLocalTime():d\\. MMM yy - HH:mm}");
                    fcInstance->FilterViewController->datasetInfoLabel->set_text(fmt::format("{} songs in dataset ", songDetails->songs.size()));
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