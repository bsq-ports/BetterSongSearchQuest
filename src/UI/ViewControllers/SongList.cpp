#include "UI/ViewControllers/SongList.hpp"
#include "Util/ResettableStaticPtr.hpp"
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
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "System/StringComparison.hpp"
#include "System/Nullable_1.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "bsml/shared/BSML.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "fmt/fmt/include/fmt/core.h"
#include "Util/SongUtil.hpp"
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
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "UI/ViewControllers/SongListCell.hpp"
#include "UI/Manager.hpp"
#include "Util/TextUtil.hpp"
#include "Util/BSMLStuff.hpp"
#include "System/Threading/Tasks/Task.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "Util/Debug.hpp"
#include <cmath>
#include "song-details/shared/SongDetails.hpp"
#include <limits>

#define coro(coroutine) SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))


using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI::Util::BSMLStuff;
using namespace GlobalNamespace;
using namespace SongDetailsCache;

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

using SortFunction = std::function< float (SongDetailsCache::Song const*)>;

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

    float songRating = song->rating();
    if(songRating < filterOptions.minRating) return false;
    
    if(((int)song->upvotes + (int)song->downvotes) < filterOptions.minVotes) return false;

    

    // Skip if not needed
    if (filterOptions.localScoreType != FilterOptions::LocalScoreFilterType::All ) {
        bool hasLocalScore = false;
        if (DataHolder::songsWithScores.contains(songHash)) {
            hasLocalScore = true;
        } 
        if (hasLocalScore) {
            if(filterOptions.localScoreType == FilterOptions::LocalScoreFilterType::HidePassed)
                return false;
        } else {
            if(filterOptions.localScoreType == FilterOptions::LocalScoreFilterType::OnlyPassed)
                return false;
        }
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


    // This is the most heavy filter, check it last
    if (filterOptions.downloadType != FilterOptions::DownloadFilterType::All) {
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
    }

    return true;
}

bool BetterSongSearch::UI::DifficultyCheck(const SongDetailsCache::SongDifficulty* diff, const SongDetailsCache::Song* song) {
    auto const& currentFilter = DataHolder::filterOptionsCache;
    if (currentFilter.skipFilter) {
        return true;
    }


    if(currentFilter.rankedType == FilterOptions::RankedFilterType::OnlyRanked)
        if(!diff->ranked())
            return false;

    if(currentFilter.rankedType != FilterOptions::RankedFilterType::HideRanked)
        if(diff->stars < currentFilter.minStars || diff->stars > currentFilter.maxStars)
            return false;

    if (currentFilter.difficultyFilter != FilterOptions::DifficultyFilterType::All) {
        if (diff->difficulty != currentFilter.difficultyFilterPreprocessed) {
            return false;
        }
    }
    
    if (currentFilter.charFilter != FilterOptions::CharFilterType::All) {
        if (diff->characteristic != currentFilter.charFilterPreprocessed) {
            return false;
        }
    }

    if(diff->njs < currentFilter.minNJS || diff->njs > currentFilter.maxNJS)
        return false;
    
    if(currentFilter.modRequirement != FilterOptions::RequirementType::Any) {
        switch (currentFilter.modRequirement)
        {
            case FilterOptions::RequirementType::Chroma:
                if (!hasFlags(diff->mods, MapMods::Chroma)) return false;
                break;
            case FilterOptions::RequirementType::Cinema:
                if (!hasFlags(diff->mods, MapMods::Cinema)) return false;
                break;
            case FilterOptions::RequirementType::MappingExtensions:
                if (!hasFlags(diff->mods, MapMods::MappingExtensions)) return false;
                break;
            case FilterOptions::RequirementType::NoodleExtensions:
                if (!hasFlags(diff->mods, MapMods::NoodleExtensions)) return false;
                break;
            case FilterOptions::RequirementType::None:
                if (!((diff->mods & (MapMods::NE | MapMods::ME)) == MapMods::None)) return false;
                break;
        default:
            break;
        }
    }

    if(song->songDurationSeconds > 0 ) {
        float nps = (float)diff->notes / (float)song->songDurationSeconds;

        if(nps < currentFilter.minNPS || nps > currentFilter.maxNPS)
            return false;
    }

    return true;
}


std::unordered_map<SortMode, SortFunction> sortFunctionMap = {
        {SortMode::Newest, [] (const SongDetailsCache::Song* x) // Newest
                           {
                               return (x->uploadTimeUnix);
                           }},
        {SortMode::Oldest, [] (const SongDetailsCache::Song* x ) // Oldest
                           {
                               return (std::numeric_limits<uint32_t>::max() - x->uploadTimeUnix);
                           }},
        {SortMode::Latest_Ranked, [] (const SongDetailsCache::Song* x) // Latest Ranked
                            {
                                return (x->rankedStatus != SongDetailsCache::RankedStatus::Unranked)? x->rankedChangeUnix: 0.0f;
                            }},
        {SortMode::Most_Stars, [] (const SongDetailsCache::Song* x) // Most Stars
                           {
                                return x->max([x](const auto& diff){
                                    bool passesFilter = DifficultyCheck(&diff, x);
                                    if (passesFilter && diff.ranked()) {
                                        return diff.stars;
                                    } else {
                                        return 0.0f;
                                    }
                                });
                           }},
        {SortMode::Least_Stars, [] (const SongDetailsCache::Song* x) // Least Stars
                           {
                               return 420.0f - x->min([x](const auto& diff){
                                    bool passesFilter = DifficultyCheck(&diff, x);
                                    if (passesFilter && diff.ranked()) {
                                        return diff.stars;
                                    } else {
                                        return 420.0f;
                                    }
                                });
                           }},
        {SortMode::Best_rated, [] (const SongDetailsCache::Song* x) // Best rated
                           {
                                return x->rating();
                           }},
        {SortMode::Worst_rated, [] (const SongDetailsCache::Song* x)//Worst rated
                            {
                                return 420.0f - (x->rating() !=0 ? x->rating(): 420.0f); 
                           }}
};

struct xd {
    const SongDetailsCache::Song * song;
	float searchWeight;
	float sortWeight;
};

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

    // Detect changes
    bool currentFilterChanged = this->filterChanged;
    bool currentSortChanged = prevSort != sort;
    bool currentSearchChanged = prevSearch != search;

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

    // Save for debugging
    DataHolder::currentSort = sort;
    DataHolder::currentSearch = search;

    // Reset values
    prevSort = sort;
    prevSearch = search;
    this->filterChanged = false;

    std::thread([this, currentSearch, currentSort, currentFilterChanged, currentSortChanged, currentSearchChanged]{
        long long before = CurrentTimeMs();


        // TODO: Actually update the scores
        // Get scores if we don't have them
        if (DataHolder::songsWithScores.size() == 0 ) {
            if(DataHolder::playerDataModel != nullptr && DataHolder::playerDataModel->m_CachedPtr.m_value != nullptr) {
                for(int i = 0; i < DataHolder::playerDataModel->get_playerData()->get_levelsStatsData()->get_Count(); i++) {
                    GlobalNamespace::PlayerLevelStatsData* x = DataHolder::playerDataModel->get_playerData()->get_levelsStatsData()->get_Item(i);
                    if(!x->validScore || x->highScore == 0 || x->levelID->get_Length() < 13 + 40)
                        continue;
                    std::u16string_view levelid = x->levelID;
                    if (!levelid.starts_with(u"custom_level_")) {
                        continue;
                    };
                    auto sh = std::regex_replace((std::string)x->levelID, std::basic_regex("custom_level_"), "");

                    auto& song = DataHolder::songDetails->songs.FindByHash(sh);

                    if(song == SongDetailsCache::Song::none)
                        continue;

                    bool foundDiff = false;

                    for (auto& diff:song) {
                        if (diff.difficulty == SongDetailsCache::MapDifficulty((int)x->difficulty)) {
                            foundDiff = true;
                            break;
                        }
                    }
                    if(!foundDiff) continue;

                    DataHolder::songsWithScores.insert(sh);
                }
                INFO("local scores checked. found {}", DataHolder::songsWithScores.size());
            }
            INFO("Checked local scores in {} ms",  CurrentTimeMs()-before);
        }

        // Prolly need 4 but gotta go fast
        const int num_threads = 8;
        std::thread t[num_threads];
        
        // Filter songs if needed
        if (currentFilterChanged) {
            DEBUG("Filtering");
            int totalSongs = DataHolder::songDetails->songs.size();
            DataHolder::filteredSongList.clear();
            if (DataHolder::filterOptionsCache.skipFilter) {
                DEBUG("Filtering skipped");
                DataHolder::filteredSongList.reserve(totalSongs);
                for (auto & song: DataHolder::songDetails->songs) {
                    DataHolder::filteredSongList.push_back(&song);
                }
            } else {
                // Set up variables for threads
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
            
        }

        INFO("Filtered in {} ms",  CurrentTimeMs()-before);

        
        if (currentFilterChanged || currentSearchChanged || currentSortChanged) {
            if (currentSearch.length() > 0) {
                auto words = split(currentSearch, " ");
                DEBUG("Words length {}", words.size());
                for (int i = 0; i < words.size(); i++) {
                    DEBUG("Search term: '{}'", words[i]);
                };

                // TODO: Process song key
                uint32_t possibleSongKey = 0;
                // Try to parse song key if 1 word from 2 to 7 letters
                if(words.size() == 1 && currentSearch.length() >= 2 && currentSearch.length() <= 7) {
                	try {
                		possibleSongKey = static_cast<uint32_t>(std::stoul(currentSearch, nullptr, 16));
                	} catch (...) {}
                }

                float maxSearchWeight = 0.0f;
                float maxSortWeight = 0.0f;
                
                DEBUG("Searching");
                long long before = CurrentTimeMs();
                DataHolder::searchedSongList.clear();
                // Set up variables for threads
                int totalSongs = DataHolder::filteredSongList.size();
                
                std::mutex valuesMutex;
                std::atomic_int index = 0;

                // Prefiltered songs
                std::vector<xd> prefiltered; 

                //Launch a group of threads
                for (int i = 0; i < num_threads; ++i) {
                    t[i] = std::thread([&index, &valuesMutex, totalSongs, currentSearch, words, &prefiltered, &maxSearchWeight, &maxSortWeight, currentSort, possibleSongKey](std::vector<std::string> searchQuery){
                        
                        int j = index++;
                        while(j < totalSongs) {
                            auto songe = DataHolder::filteredSongList[j];
                            

                            float resultWeight = 0;
                            bool matchedAuthor = false;
                            int prevMatchIndex = -1;


                            std::string songName = removeSpecialCharacter(toLower(songe->songName()));
                            std::string songAuthorName = removeSpecialCharacter(toLower(songe->songAuthorName()));
                            std::string levelAuthorName = removeSpecialCharacter(toLower(songe->levelAuthorName()));
                            uint32_t songKey = songe->mapId();

                            // If song key is present and mapid == songkey, pull it to the top
                            if(possibleSongKey != 0 && songKey == possibleSongKey)
                                resultWeight = 30;

                            // Find full match author name
                            int authorFullMatch = currentSearch.find(songAuthorName);

                            // set up i for the loop
                            int i = 0;
                            
                            if(songAuthorName.length() > 4 && authorFullMatch != std::string::npos && 
                                // Checks if there is a space after the supposedly matched author name
                                (currentSearch.length() == songAuthorName.length() || IsSpace(currentSearch[songAuthorName.length()]))
                            ) {
                                matchedAuthor = true;
                                resultWeight += songAuthorName.length() > 5 ? 25 : 20;

                                // If the author is matched and is the first, then skip first word (i + 1)
                                // This is super cheapskate - I'd have to replace the author from the filter and recreate the words array otherwise
                                if(authorFullMatch == 0)
                                    i = 1;
                            }

                            // Go over a list of words
                            for (; i < words.size(); i++) {
                                // If the word matches the author 1:1 thats cool innit
                                // If author name is not empty
                                if (songAuthorName.length() != 0) {
                                    // If not matched author and author name == word then add weight, skip if author is already matched
                                    if(!matchedAuthor && songAuthorName == words[i]) {
                                        matchedAuthor = true;
                                        // 3*length of the word divided by 2? wtf
                                        resultWeight += 3 * (words[i].length() / 2);

                                        // Go to next word
                                        continue;
                                        // Otherwise we'll have to check if its contained within this word
                                    } else if(!matchedAuthor && words[i].length() >= 3) {
                                        int index = songAuthorName.find(words[i]);

                                        // If found in the beginning or is space at the end of author name which means we matched the beginning of a word
                                        if(index == 0 || (index > 0 && IsSpace(songAuthorName[index - 1]))) {
                                            
                                            matchedAuthor = true;
                                            // Add weight
                                            resultWeight += (int)round((index == 0 ? 4.0f : 3.0f) * ((float)words[i].length() / songAuthorName.length()));
                                            continue;
                                        }
                                    }
                                }

                                int matchpos = songName.find(words[i]);
                                if(matchpos != std::string::npos) {
                                    // Check if we matched the beginning of a word
                                    bool wordStart = matchpos == 0 || songName[matchpos - 1] == ' ';

                                    // If it was the beginning add 5 weighting, else 3
                                    resultWeight += wordStart ? 5 : 3;
                                    

                                    ///////////////// New algo  /////////////////////////
                                    // Find the position in the name
                                    int posInName = matchpos + words[i].length();

                                    /*
                                    * Check if we are at the end of the song name, but only if it has at least 8 characters
                                    * We do this because otherwise, when searching for "lowermost revolt", songs where the
                                    * songName is exactly "lowermost revolt" would have a lower result weight than 
                                    * "lowermost revolt (JoeBama cover)"
                                    * 
                                    * The 8 character limitation for this is so that super short words like "those" dont end
                                    * up triggering this
                                    */
                                    if(songName.length() > 7 && songName.length() == posInName) {
                                        resultWeight += 3;
                                    } else {
                                        // If we did match the beginning, check if we matched an entire word. Get the end index as indicated by our needle
                                        bool maybeWordEnd = wordStart && posInName < songName.length();

                                        // Check if we actually end up at a non word char, if so add 2 weighting
                                        if(maybeWordEnd && songName[matchpos + words[i].length()] == ' ')
                                            resultWeight += 2;
                                    }
                                    /////////////////////////////////////////////////////

                                    //// Old algo for testing pc compatibility (comment out new algo and uncomment this for comparison with PC) //////////
                                    // bool maybeWordEnd = wordStart && matchpos + words[i].length() < songName.length();

                                    // // Check if we actually end up at a non word char, if so add 2 weighting
                                    // if(maybeWordEnd && songName[matchpos + words[i].length()] == ' ')
                                    //     resultWeight += 2;
                                    ////////////////////////////////////////////////////


                                    // If the word we just checked is behind the previous matched, add another 1 weight
                                    if(prevMatchIndex != -1 && matchpos > prevMatchIndex)
                                        resultWeight += 1;

                                    prevMatchIndex = matchpos;
                                }
                            }

                            for(i = 0; i < words.size(); i++) {
                                if(words[i].length() > 3 && levelAuthorName.find(words[i]) != std::string::npos) {
                                    resultWeight += 1;

                                    break;
                                }
                            }

                            if(resultWeight > 0) {
                                float sortWeight = sortFunctionMap.at(currentSort)(songe);

                                std::lock_guard<std::mutex> lock(valuesMutex);

                                prefiltered.push_back({
                                    songe, resultWeight, sortWeight
                                });

                                // #if DEBUG
                                //                         x.sortWeight = sortWeight;
                                //                         x.resultWeight = resultWeight;
                                // #endif
                                if(maxSearchWeight < resultWeight)
                                    maxSearchWeight = resultWeight;

                                if(maxSortWeight < sortWeight)
                                    maxSortWeight = sortWeight;
                            }
                            j = index++;
                        }
                    }, words);
                }

                //Join the threads with the main thread
                for (int i = 0; i < num_threads; ++i) { t[i].join(); }

                INFO("Calculated search indexes in {} ms",  CurrentTimeMs()-before);
                if (prefiltered.size() == 0) {
                    DataHolder::searchedSongList.clear();
                } else {
                    long long before = CurrentTimeMs();
                    float maxSearchWeightInverse = 1.0f / maxSearchWeight;
                    float maxSortWeightInverse = 1.0f / maxSortWeight;

                    // Calculate total search weight
                    for (auto& item :prefiltered) {
                        float searchWeight = item.searchWeight * maxSearchWeightInverse;
                        item.searchWeight = searchWeight + min(searchWeight / 2, item.sortWeight * maxSortWeightInverse * (searchWeight / 2));
                    }
                    
                    
                    std::stable_sort(prefiltered.begin(), prefiltered.end(),
                        [](xd s1, xd s2){
                            return s1.searchWeight > s2.searchWeight;
                        }
                    );

                    for (auto x: prefiltered) {
                        DataHolder::searchedSongList.push_back(x.song);
                    }
                    INFO("sorted search results in {} ms",  CurrentTimeMs()-before);
                }
            } else {
                long long before = CurrentTimeMs();

                std::vector<xd> prefiltered; 
                auto sortFunction = sortFunctionMap.at(currentSort);
                for (auto item : DataHolder::filteredSongList) {
                    auto score = sortFunction(item);
                    prefiltered.push_back({
                        item, 0 , score 
                    });
                }

                std::stable_sort(
                    prefiltered.begin(),
                    prefiltered.end(),
                    [](const xd& s1, const xd& s2){
                        return s1.sortWeight > s2.sortWeight;
                    }
                );

                DataHolder::searchedSongList.clear();
                DataHolder::searchedSongList.reserve(DataHolder::filteredSongList.size());

                // Push to searched
                for (auto x: prefiltered) {
                    DataHolder::searchedSongList.push_back(x.song);
                }
              
                INFO("Sort without search in {} ms",  CurrentTimeMs()-before);
            }
        }

        
        DEBUG("Search time: {}ms", CurrentTimeMs()-before);
        QuestUI::MainThreadScheduler::Schedule([this]{
            long long before = 0;
            before = CurrentTimeMs();
            
            // Copy the list to the displayed one
            DataHolder::sortedSongList.clear();
            DataHolder::sortedSongList = DataHolder::searchedSongList;
            this->ResetTable();

            INFO("table reset in {} ms",  CurrentTimeMs()-before);

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

    // Set default cover image
    coverImage->set_sprite(defaultImage);

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

    // If needs a refresh, refresh when shown
    if (DataHolder::loaded && DataHolder::needsRefresh) {
        // Initial search
        this->filterChanged = true;
        fcInstance->SongListController->SortAndFilterSongs(this->sort, this->search, true);
        fcInstance->FilterViewController->datasetInfoLabel->set_text(fmt::format("{} songs in dataset ", DataHolder::songDetails->songs.size()));
    }

    if (!firstActivation)
        return;

    if (DataHolder::playerDataModel == nullptr || DataHolder::playerDataModel->m_CachedPtr.m_value == nullptr) {
        DataHolder::playerDataModel =  UnityEngine::GameObject::FindObjectOfType<GlobalNamespace::PlayerDataModel*>();
    };
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

    // If loaded, refresh
    if (DataHolder::loaded) {
        DEBUG("Loaded is true");
        // Initial search
        this->filterChanged = true;
        fcInstance->SongListController->SortAndFilterSongs(this->sort, this->search, true);
        fcInstance->FilterViewController->datasetInfoLabel->set_text(fmt::format("{} songs in dataset ", DataHolder::songDetails->songs.size()));
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
    
    // Sub to events
    SongDetailsCache::SongDetails::dataAvailableOrUpdated += {&ViewControllers::SongListController::SongDataDone ,this};
    SongDetailsCache::SongDetails::dataLoadFailed += {&ViewControllers::SongListController::SongDataError ,this};
}

void ViewControllers::SongListController::dtor()
{
    // Unsub from events
    SongDetailsCache::SongDetails::dataAvailableOrUpdated -= {&ViewControllers::SongListController::SongDataDone ,this};
    SongDetailsCache::SongDetails::dataLoadFailed -= {&ViewControllers::SongListController::SongDataError ,this};
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
    
    // get song loader object
    STATIC_AUTO(songLoaderObject, UnityEngine::Resources::FindObjectsOfTypeAll(il2cpp_utils::GetSystemType("RuntimeSongLoader", "SongLoader")).First());
    // get custom levels pack field
    STATIC_AUTO(customLevelsPack, CRASH_UNLESS(il2cpp_utils::GetFieldValue<RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack*>(songLoaderObject, "CustomLevelsPack")));

    // auto customLevelsPack = RuntimeSongLoader::API::GetCustomLevelsPack();
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

    // For some reason setup does not work for multiplayer so I have to use this method to workaround
    multiplayerLevelSelectionFlowCoordinator->levelSelectionNavigationController->levelCollectionNavigationController->SelectLevel(level);
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
    
    // Get old sprite if it exists
    UnityEngine::Sprite* oldSprite = this->coverImage->get_sprite();

    auto song = currentSong;
    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->hash()));
    bool loaded = beatmap.has_value();
    bool downloaded = fcInstance->DownloadHistoryViewController->CheckIsDownloaded(std::string(song->hash()));
    #ifdef SONGDOWNLOADER
    
    // if beatmap is loaded
    if (loaded) {
        auto cover = BetterSongSearch::Util::getLocalCoverSync(beatmap.value());
        if (cover != nullptr) {
            this->coverImage->set_sprite(cover);
        } else {
            this->coverImage->set_sprite(defaultImage);
            
            // Cleanup old sprite
            if (
                // Don't delete defaultImage
                oldSprite != defaultImage && 
                this->coverImage->get_sprite() != oldSprite)
            {
                    if (oldSprite != nullptr && oldSprite->m_CachedPtr.m_value != nullptr) {
                        DEBUG("REMOVING OLD SPRITE");
                        auto texture = oldSprite->get_texture();
                        if (texture != nullptr && texture->m_CachedPtr.m_value != nullptr) {
                            UnityEngine::Object::DestroyImmediate(texture);
                        }
                        UnityEngine::Object::DestroyImmediate(oldSprite);
                    } 
            }
        }
        coverLoading->set_enabled(false);
    } else {
        
        std::string newUrl = fmt::format("{}/{}.jpg", BeatSaverRegionManager::coverDownloadUrl, toLower(song->hash()));
        DEBUG("{}", newUrl.c_str());
        coverLoading->set_enabled(true);
        GetByURLAsync(newUrl, [this, song, oldSprite](bool success, std::vector<uint8_t> bytes) {
            QuestUI::MainThreadScheduler::Schedule([this, bytes, song, success, oldSprite] {
                if (success) {
                    std::vector<uint8_t> data = bytes;
                    DEBUG("Image size: {}", pretty_bytes(bytes.size()));
                    if (song->hash() != this->currentSong->hash()) return;
                    Array<uint8_t> *spriteArray = il2cpp_utils::vectorToArray(data);
                    this->coverImage->set_sprite(QuestUI::BeatSaberUI::ArrayToSprite(spriteArray));
                } else {
                    this->coverImage->set_sprite(defaultImage);
                }
                coverLoading->set_enabled(false);

                // Cleanup old sprite
                if (
                    // Don't delete old image if it's a default image
                    oldSprite != defaultImage && 
                    this->coverImage->get_sprite() != oldSprite)
                {
                    if (oldSprite != nullptr && oldSprite->m_CachedPtr.m_value != nullptr) {
                        auto texture = oldSprite->get_texture();
                        if (texture != nullptr && texture->m_CachedPtr.m_value != nullptr) {
                            UnityEngine::Object::DestroyImmediate(texture);
                        }
                        UnityEngine::Object::DestroyImmediate(oldSprite);
                    } 
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

    // LogSongInfo(song);

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
    DEBUG("DownloadSongList");
    if (DataHolder::loading) {
        return;
    }

    fcInstance->FilterViewController->datasetInfoLabel->set_text("Loading dataset...");

    std::thread([this]{
        DataHolder::loading = true;
        DEBUG("Getting songdetails");
        DataHolder::songDetails = SongDetailsCache::SongDetails::Init().get();
        DEBUG("Got songdetails");


        if (!DataHolder::songDetails->songs.get_isDataAvailable()) {
            this->SongDataError();
        } else {
            this->SongDataDone();
        }
    }).detach();
}

void ViewControllers::SongListController::RetryDownloadSongList() {
    if (DataHolder::failed && !DataHolder::loading && !DataHolder::loaded) {
        this->DownloadSongList();
    }
}


// Event receivers
void ViewControllers::SongListController::SongDataDone() {
    DEBUG("SongDataDone");
    // Set state flags
    DataHolder::loading = false;
    DataHolder::failed = false;
    DataHolder::loaded = true;
    DataHolder::invalid = false;
    DataHolder::needsRefresh = false;

    QuestUI::MainThreadScheduler::Schedule([this]{

        if (this->get_isActiveAndEnabled()) {
            // Initial search
            this->filterChanged = true;
            fcInstance->SongListController->SortAndFilterSongs(this->sort, this->search, true);
            // filterView.datasetInfoLabel?.SetText($"{songDetails.songs.Length} songs in dataset | Newest: {songDetails.songs.Last().uploadTime.ToLocalTime():d\\. MMM yy - HH:mm}");
            fcInstance->FilterViewController->datasetInfoLabel->set_text(fmt::format("{} songs in dataset ", DataHolder::songDetails->songs.size()));
        } else {
            DataHolder::needsRefresh = true;
        }
    });
}

void ViewControllers::SongListController::SongDataError() {
    DEBUG("SongDataError");

    // Set state flags
    DataHolder::loading = false;
    DataHolder::failed = true;
    DataHolder::loaded = false;
    DataHolder::invalid = true;
    DataHolder::needsRefresh = false;
    
    QuestUI::MainThreadScheduler::Schedule([this]{
        if (this->m_CachedPtr.m_value != nullptr ) {  
            fcInstance->FilterViewController->datasetInfoLabel->set_text("Failed to load, click to retry");
        }
    });
}