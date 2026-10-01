#include "DataHolder.hpp"

#include <array>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <utility>

#include "beatsaber-hook/shared/threading.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "logging.hpp"
#include "song-details/shared/Data/Song.hpp"
#include "song-details/shared/Data/SongDifficulty.hpp"
#include "song-details/shared/SongDetails.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "Util/SearchTextCache.hpp"
#include "Util/SongUtil.hpp"
#include "Util/TextUtil.hpp"

using namespace BetterSongSearch::Util;

// Initialize the data holder
BetterSongSearch::DataHolder BetterSongSearch::dataHolder{};

void BetterSongSearch::DataHolder::Init() {
    // Subscribe to events
    SongDetailsCache::SongDetails::dataAvailableOrUpdated += {&DataHolder::SongDataDone, this};
    SongDetailsCache::SongDetails::dataLoadFailed += {&DataHolder::SongDataError, this};

    // Load configs
    dataHolder.filterOptions.LoadFromConfig();
    dataHolder.filterOptionsCache.CopyFrom(dataHolder.filterOptions);

    // set preferred leaderboard
    std::string preferredLeaderboard = getPluginConfig().PreferredLeaderboard.GetValue();
    if (LEADERBOARD_MAP.contains(preferredLeaderboard)) {
        dataHolder.preferredLeaderboard = LEADERBOARD_MAP.at(preferredLeaderboard);
    } else {
        dataHolder.preferredLeaderboard = FilterTypes::PreferredLeaderBoard::ScoreSaber;
        getPluginConfig().PreferredLeaderboard.SetValue("Scoresaber");
    }
}

void BetterSongSearch::DataHolder::SongDataDone() {
    DEBUG("SongDataDone");
    // Needed if songdetails is loaded in the background by other mods.
    if (this->songDetails == nullptr) {
        this->songDetails = SongDetailsCache::SongDetails::Init().get();
    }
    _songDataGeneration.fetch_add(1, std::memory_order_acq_rel);
    std::atomic_store_explicit(&_searchTextCache, std::shared_ptr<SearchTextCache const>{}, std::memory_order_release);
    PreprocessTags();
    UpdatePlayerScores();

    loading = false;
    failed = false;
    loaded = true;
    needsRefresh = true;

    loadingFinished.invoke();
}

void BetterSongSearch::DataHolder::SongDataError(std::string message) {
    DEBUG("SongDataFailed");

    loading = false;
    failed = true;
    loaded = false;
    needsRefresh = false;

    loadingFailed.invoke(message);
}

void BetterSongSearch::DataHolder::DownloadSongList() {
    DEBUG("DownloadSongList");
    if (this->loading) {
        return;
    }

    std::thread([this] {
        this->loading = true;
        this->songDetails = SongDetailsCache::SongDetails::Init().get();

        if (!this->songDetails->songs.get_isDataAvailable()) {
            this->SongDataError("Failed to load song data");
        } else {
            this->SongDataDone();
        }
    }).detach();
}

void BetterSongSearch::DataHolder::PreprocessTags() {
    auto& mapStyles = MAP_STYLES_OPTIONS;

    std::vector<PreprocessedTag> tags;

    for (auto const& tag : songDetails->tags) {
        auto const& tagString = tag.first;
        uint64_t mask = tag.second;

        bool isStyle = mapStyles.end() != std::find(mapStyles.begin(), mapStyles.end(), tagString);

        if (isStyle) {
            continue;
        }

        // Count songs
        uint32_t songCount = 0;
        for (auto& song : songDetails->songs) {
            if ((song.tags & mask) == mask) {
                songCount++;
            }
        }

        tags.push_back({tagString, mask, songCount, false});
    }

    // Sort by alphabetical order
    std::sort(tags.begin(), tags.end(), [](PreprocessedTag const& a, PreprocessedTag const& b) {
        return a.tag < b.tag;
    });

    // Replace the tags with the preprocessed ones
    this->tags = std::move(tags);
}

void BetterSongSearch::DataHolder::UpdatePlayerScores() {
    static std::atomic_bool updating = false;  // Prevent multiple checks at once
    if (updating.exchange(true)) {
        return;
    }

    il2cpp_thread([this] {
        try {
            DEBUG("Updating player scores");
            long long before = CurrentTimeMs();

            if (!playerDataModel) {
                WARNING("No player data model, cannot update scores");
                updating.store(false);
                return;
            }

            auto playerData = playerDataModel->get_playerData();
            if (!playerData) {
                WARNING("No player data, cannot update scores");
                updating.store(false);
                return;
            }

            auto statsData = playerData->get_levelsStatsData();
            if (!statsData) {
                WARNING("No stats data, cannot update scores");
                updating.store(false);
                return;
            }

            if (!this->songDetails || !this->songDetails->songs.get_isDataAvailable()) {
                WARNING("No song details, cannot update scores");
                updating.store(false);
                return;
            }

            auto statsDataEnumerator = statsData->GetEnumerator();

            auto songsWithScoresTemp = std::make_shared<SongScores>();
            songsWithScoresTemp->reserve(statsData->get_Count());

            while (statsDataEnumerator.MoveNext()) {
                auto statsDataKeys = statsDataEnumerator.get_Current();
                auto x = statsDataKeys.value;

                if (!x->_validScore || x->_highScore == 0 || x->_levelID->get_Length() < 13 + 40) {
                    continue;
                }
                std::u16string_view levelid = x->_levelID;
                if (!levelid.starts_with(u"custom_level_")) {
                    continue;
                };
                auto sh = static_cast<std::string>(x->_levelID);
                sh.erase(0, 13);
                if (songsWithScoresTemp->contains(sh)) {
                    continue;
                }

                auto& song = dataHolder.songDetails->songs.FindByHash(sh);

                if (song == SongDetailsCache::Song::none) {
                    continue;
                }

                bool foundDiff = false;

                for (auto& diff : song) {
                    if (diff.difficulty == SongDetailsCache::MapDifficulty((int) x->_difficulty.value__)) {
                        foundDiff = true;
                        break;
                    }
                }
                if (!foundDiff) {
                    continue;
                }

                songsWithScoresTemp->insert(std::move(sh));
            }
            INFO("local scores checked. found {}", songsWithScoresTemp->size());

            bool isChanged = PublishScoreSnapshot(std::move(songsWithScoresTemp));

            updating.store(false);

            INFO("Updated player scores in {} ms", CurrentTimeMs() - before);

            if (isChanged) {
                BSML::MainThreadScheduler::Schedule([this] {
                    playerDataLoaded.invoke();
                    if (!this->searchInProgress && this->filterOptions.getLocalScoreType() != FilterTypes::LocalScoreFilter::All) {
                        this->Search();
                    }
                });
            }
        } catch (...) {
            WARNING("Failed to update player scores");
            updating.store(false);
        }
    }).detach();
}

bool BetterSongSearch::DataHolder::SongHasScore(std::string_view songhash) {
    return GetScoreSnapshot()->contains(songhash);
}

std::shared_ptr<BetterSongSearch::Util::SongScores const> BetterSongSearch::DataHolder::GetScoreSnapshot() const {
    return std::atomic_load_explicit(&songsWithScores, std::memory_order_acquire);
}

bool BetterSongSearch::DataHolder::PublishScoreSnapshot(std::shared_ptr<SongScores const> scores) {
    auto previousScores = GetScoreSnapshot();
    if (*previousScores == *scores) {
        return false;
    }
    std::atomic_store_explicit(&songsWithScores, std::move(scores), std::memory_order_release);
    return true;
}

bool BetterSongSearch::DataHolder::SongHasScore(SongDetailsCache::Song const* song) {
    return SongHasScore(song->hash());
}

void BetterSongSearch::DataHolder::SongListUIDone() {
    this->searchInProgress = false;
}

struct xd {
    SongDetailsCache::Song const* song;
    float searchWeight;
    float sortWeight;
};

std::shared_ptr<BetterSongSearch::Util::SearchTextCache const> BetterSongSearch::DataHolder::GetSearchTextCache() {
    auto generation = _songDataGeneration.load(std::memory_order_acquire);
    auto cached = std::atomic_load_explicit(&_searchTextCache, std::memory_order_acquire);
    if (cached && cached->generation == generation && cached->SongCount() == songDetails->songs.size()) {
        return cached;
    }

    auto cache = std::make_shared<SearchTextCache>(generation);
    std::size_t characters = 0;
    for (auto const& song : songDetails->songs) {
        characters += song.songName().size() + song.songAuthorName().size() + song.levelAuthorName().size();
    }
    cache->Reserve(songDetails->songs.size(), characters);
    for (auto const& song : songDetails->songs) {
        auto songName = NormalizeSearchText(song.songName());
        auto songAuthorName = NormalizeSearchText(song.songAuthorName());
        auto levelAuthorName = NormalizeSearchText(song.levelAuthorName());
        cache->Append(songName, songAuthorName, levelAuthorName);
    }
    if (_songDataGeneration.load(std::memory_order_acquire) == generation) {
        std::atomic_store_explicit(&_searchTextCache, std::shared_ptr<SearchTextCache const>(cache), std::memory_order_release);
    }
    INFO("Cached search text for {} songs using {} bytes", cache->SongCount(), cache->MemoryBytes());
    return cache;
}

void BetterSongSearch::DataHolder::Search() {
    DEBUG("BetterSongSearch::DataHolder::Search called");
    // Skip if song details is null or if data is not loaded yet
    if (this->songDetails == nullptr || !this->songDetails->songs.get_isDataAvailable()) {
        DEBUG("Skipping search as song details are not loaded yet");
        return;
    };

    // Skip if we have no songs
    int totalSongs = this->songDetails->songs.size();
    DEBUG("Total songs: {}", totalSongs);
    if (totalSongs == 0) {
        return;
    }
    if (this->searchInProgress) {
        return;
    }

    this->searchInProgress = true;

    // Detect changes
    bool currentSortChanged = this->sort != this->currentSort;
    bool currentSearchChanged = this->search != this->currentSearch;
    auto scoreSnapshot = GetScoreSnapshot();
    bool currentFilterChanged = !this->filterOptionsCache.IsEqual(this->filterOptions) ||
                               (this->filterOptions.getLocalScoreType() != FilterTypes::LocalScoreFilter::All && scoreSnapshot != _filteredScoreSnapshot);
    bool currentForceReload = this->forceReload;
    DEBUG(
        "Current sort changed: {}, current search changed: {}, filter changed: {}, force reload: {}",
        currentSortChanged,
        currentSearchChanged,
        currentFilterChanged,
        currentForceReload
    );
    if (!currentForceReload && !currentSortChanged && !currentSearchChanged && !currentFilterChanged) {
        this->searchInProgress = false;
        DEBUG("Skipping search as nothing changed");
        return;
    }

    if (currentForceReload) {
        DEBUG("Force reload");
        this->forceReload = false;
    }

    // Take a snapshot of current filter options
    this->filterOptionsCache.CopyFrom(this->filterOptions);

    // Calculate temp values
    this->filterOptionsCache.RecalculatePreprocessedValues();

    DEBUG("SEARCHING");
    this->filterOptionsCache.PrintToDebug();

    // Grab current values for sort and search
    auto currentSort = this->sort;
    // Current search (processed)
    auto currentSearch = toLower(this->search);

    // Save
    this->currentSort = this->sort;
    this->currentSearch = this->search;

    std::thread([this, currentSearch = std::move(currentSearch), scoreSnapshot = std::move(scoreSnapshot), currentSort, currentFilterChanged, currentSortChanged, currentSearchChanged, currentForceReload] {
        long long before = CurrentTimeMs();

        // 4 threads are fine
        int const num_threads = 4;
        std::thread t[num_threads];

        // Filter songs if needed
        if (currentFilterChanged || currentForceReload) {
            DEBUG("Filtering");
            int totalSongs = this->songDetails->songs.size();
            this->_filteredSongList.clear();
            if (this->filterOptionsCache.IsDefault()) {
                DEBUG("Filtering skipped");
                this->_filteredSongList.reserve(totalSongs);
                for (auto& song : this->songDetails->songs) {
                    this->_filteredSongList.push_back(&song);
                }
            } else {
                this->_filteredSongList.resize(totalSongs);

                // Launch a group of threads
                for (int i = 0; i < num_threads; ++i) {
                    int startIndex = totalSongs * i / num_threads;
                    int endIndex = totalSongs * (i + 1) / num_threads;
                    t[i] = std::thread([startIndex, endIndex, this, &scoreSnapshot]() {
                        for (int j = startIndex; j < endIndex; ++j) {
                            SongDetailsCache::Song const& item = this->songDetails->songs.at(j);
                            _filteredSongList[j] = MeetsFilter(&item, scoreSnapshot.get()) ? &item : nullptr;
                        }
                    });
                }

                // Join the threads with the main thread
                for (int i = 0; i < num_threads; ++i) {
                    t[i].join();
                }
                std::erase(this->_filteredSongList, nullptr);
            }
            _filteredScoreSnapshot = this->filterOptionsCache.getLocalScoreType() == FilterTypes::LocalScoreFilter::All ? nullptr : scoreSnapshot;
        }

        INFO("Filtered in {} ms", CurrentTimeMs() - before);

        if (currentFilterChanged || currentSearchChanged || currentSortChanged || currentForceReload) {
            if (currentSearch.length() > 0) {
                auto searchTextCache = GetSearchTextCache();
                auto words = split(currentSearch, " ");
                DEBUG("Words length {}", words.size());
                for (int i = 0; i < words.size(); i++) {
                    DEBUG("Search term: '{}'", words[i]);
                };

                // TODO: Process song key
                uint32_t possibleSongKey = 0;
                // Try to parse song key if 1 word from 2 to 7 letters
                if (words.size() == 1 && currentSearch.length() >= 2 && currentSearch.length() <= 7) {
                    try {
                        possibleSongKey = static_cast<uint32_t>(std::stoul(currentSearch, nullptr, 16));
                    } catch (...) {
                    }
                }

                std::array<float, num_threads> maxSearchWeights{};
                std::array<float, num_threads> maxSortWeights{};
                auto sortFunction = sortFunctionMap.at(currentSort);

                DEBUG("Searching");
                long long before = CurrentTimeMs();
                this->_searchedSongList.clear();
                // Set up variables for threads
                int totalSongs = this->_filteredSongList.size();

                // Prefiltered songs
                std::vector<xd> prefiltered(totalSongs);

                // Launch a group of threads
                for (int i = 0; i < num_threads; ++i) {
                    int startIndex = totalSongs * i / num_threads;
                    int endIndex = totalSongs * (i + 1) / num_threads;
                    t[i] = std::thread(
                        [this,
                         startIndex,
                         endIndex,
                         &currentSearch,
                         &words,
                         &searchTextCache,
                         &prefiltered,
                         &maxSearchWeight = maxSearchWeights[i],
                         &maxSortWeight = maxSortWeights[i],
                         sortFunction,
                         possibleSongKey] {
                            for (int j = startIndex; j < endIndex; ++j) {
                                auto songe = this->_filteredSongList[j];

                                float resultWeight = 0;
                                bool matchedAuthor = false;
                                int prevMatchIndex = -1;

                                auto text = searchTextCache->Get(songe->index);
                                auto songName = text.songName;
                                auto songAuthorName = text.songAuthorName;
                                auto levelAuthorName = text.levelAuthorName;
                                uint32_t songKey = songe->mapId();

                                // If song key is present and mapid == songkey, pull it to the top
                                if (possibleSongKey != 0 && songKey == possibleSongKey) {
                                    resultWeight = 30;
                                }

                                // Find full match author name
                                int authorFullMatch = currentSearch.find(songAuthorName);

                                // set up i for the loop
                                int i = 0;

                                if (songAuthorName.length() > 4 && authorFullMatch != std::string::npos &&
                                    // Checks if there is a space after the supposedly matched author name
                                    (currentSearch.length() == songAuthorName.length() || IsSpace(currentSearch[songAuthorName.length()]))) {
                                    matchedAuthor = true;
                                    resultWeight += songAuthorName.length() > 5 ? 25 : 20;

                                    // If the author is matched and is the first, then skip first word (i + 1)
                                    // This is super cheapskate - I'd have to replace the author from the filter and recreate the words array otherwise
                                    if (authorFullMatch == 0) {
                                        i = 1;
                                    }
                                }

                                // Go over a list of words
                                for (; i < words.size(); i++) {
                                    // If the word matches the author 1:1 thats cool innit
                                    // If author name is not empty
                                    if (songAuthorName.length() != 0) {
                                        // If not matched author and author name == word then add weight, skip if author is already matched
                                        if (!matchedAuthor && songAuthorName == words[i]) {
                                            matchedAuthor = true;
                                            // 3*length of the word divided by 2? wtf
                                            resultWeight += 3.0f * ((float) words[i].length() / 2.0f);

                                            // Go to next word
                                            continue;
                                            // Otherwise we'll have to check if its contained within this word
                                        } else if (!matchedAuthor && words[i].length() >= 3) {
                                            int index = songAuthorName.find(words[i]);

                                            // If found in the beginning or is space at the end of author name which means we matched the beginning of
                                            // a word
                                            if (index == 0 || (index > 0 && IsSpace(songAuthorName[index - 1]))) {
                                                matchedAuthor = true;
                                                // Add weight
                                                resultWeight +=
                                                    (int) round((index == 0 ? 4.0f : 3.0f) * ((float) words[i].length() / songAuthorName.length()));
                                                continue;
                                            }
                                        }
                                    }

                                    int matchpos = songName.find(words[i]);
                                    if (matchpos != std::string::npos) {
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
                                        if (songName.length() >= 6 && songName.length() == posInName) {
                                            resultWeight += 3;
                                        } else {
                                            // If we did match the beginning, check if we matched an entire word. Get the end index as indicated by
                                            // our needle
                                            bool maybeWordEnd = wordStart && posInName < songName.length();

                                            // Check if we actually end up at a non word char, if so add 2 weighting
                                            if (maybeWordEnd && songName[matchpos + words[i].length()] == ' ') {
                                                resultWeight += 2;
                                            }
                                        }
                                        /////////////////////////////////////////////////////

                                        //// Old algo for testing pc compatibility (comment out new algo and uncomment this for comparison with PC)
                                        /////////////
                                        // bool maybeWordEnd = wordStart && matchpos + words[i].length() < songName.length();

                                        // // Check if we actually end up at a non word char, if so add 2 weighting
                                        // if(maybeWordEnd && songName[matchpos + words[i].length()] == ' ')
                                        //     resultWeight += 2;
                                        ////////////////////////////////////////////////////

                                        // If the word we just checked is behind the previous matched, add another 1 weight
                                        if (prevMatchIndex != -1 && matchpos > prevMatchIndex) {
                                            resultWeight += 1;
                                        }

                                        prevMatchIndex = matchpos;
                                    }
                                }

                                for (i = 0; i < words.size(); i++) {
                                    if (words[i].length() > 3 && levelAuthorName.find(words[i]) != std::string::npos) {
                                        resultWeight += 1;
                                        break;
                                    }
                                }

                                if (resultWeight > 0) {
                                    float sortWeight = sortFunction(songe);
                                    prefiltered[j] = {songe, resultWeight, sortWeight};

                                    // #if DEBUG
                                    //                         x.sortWeight = sortWeight;
                                    //                         x.resultWeight = resultWeight;
                                    // #endif
                                    if (maxSearchWeight < resultWeight) {
                                        maxSearchWeight = resultWeight;
                                    }

                                    if (maxSortWeight < sortWeight) {
                                        maxSortWeight = sortWeight;
                                    }
                                }
                            }
                        }
                    );
                }

                // Join the threads with the main thread
                for (int i = 0; i < num_threads; ++i) {
                    t[i].join();
                }
                std::erase_if(prefiltered, [](xd const& item) { return item.song == nullptr; });
                float maxSearchWeight = *std::max_element(maxSearchWeights.begin(), maxSearchWeights.end());
                float maxSortWeight = *std::max_element(maxSortWeights.begin(), maxSortWeights.end());

                INFO("Calculated search indexes in {} ms", CurrentTimeMs() - before);
                if (prefiltered.size() == 0) {
                    this->_searchedSongList.clear();
                } else {
                    long long before = CurrentTimeMs();
                    float maxSearchWeightInverse = 1.0f / maxSearchWeight;
                    float maxSortWeightInverse = 1.0f / maxSortWeight;

                    // Calculate total search weight
                    for (auto& item : prefiltered) {
                        float searchWeight = item.searchWeight * maxSearchWeightInverse;
                        item.searchWeight = searchWeight + std::min(searchWeight / 2, item.sortWeight * maxSortWeightInverse * (searchWeight / 2));
                    }

                    std::stable_sort(prefiltered.begin(), prefiltered.end(), [](xd const& s1, xd const& s2) {
                        return s1.searchWeight > s2.searchWeight;
                    });

                    this->_searchedSongList.reserve(prefiltered.size());
                    for (auto& x : prefiltered) {
                        this->_searchedSongList.push_back(x.song);
                    }
                    INFO("sorted search results in {} ms", CurrentTimeMs() - before);
                }
            } else {
                long long before = CurrentTimeMs();

                std::vector<xd> prefiltered;
                prefiltered.reserve(this->_filteredSongList.size());
                auto sortFunction = sortFunctionMap.at(currentSort);
                for (auto item : _filteredSongList) {
                    auto score = sortFunction(item);
                    prefiltered.push_back({item, 0, score});
                }

                std::stable_sort(prefiltered.begin(), prefiltered.end(), [](xd const& s1, xd const& s2) {
                    return s1.sortWeight > s2.sortWeight;
                });

                this->_searchedSongList.clear();
                this->_searchedSongList.reserve(this->_filteredSongList.size());

                // Push to searched
                for (auto& x : prefiltered) {
                    this->_searchedSongList.push_back(x.song);
                }

                INFO("Sort without search in {} ms", CurrentTimeMs() - before);
            }
        }

        DEBUG("Search time: {}ms", CurrentTimeMs() - before);
        DEBUG("Found {} songs", _searchedSongList.size());

        // Publish results without copying the vector while holding the UI's mutex.
        std::unique_lock<std::shared_mutex> lock(_displayedSongListMutex);
        this->_displayedSongList.swap(this->_searchedSongList);
        lock.unlock();

        // Replace the list with the searched one in the main thread to prevent unsafe stuff
        BSML::MainThreadScheduler::Schedule([this, scoreSnapshot = std::move(scoreSnapshot)] {
            this->searchInProgress = false;
            this->searchEnded.invoke();
            if (this->filterOptions.getLocalScoreType() != FilterTypes::LocalScoreFilter::All && scoreSnapshot != GetScoreSnapshot()) {
                this->Search();
            }
        });
    }).detach();
}

std::vector<SongDetailsCache::Song const*> BetterSongSearch::DataHolder::GetDisplayedSongList() {
    std::shared_lock<std::shared_mutex> lock(_displayedSongListMutex);
    return this->_displayedSongList;
}

SongDetailsCache::Song const* BetterSongSearch::DataHolder::GetDisplayedSongByIndex(std::size_t index) {
    std::shared_lock<std::shared_mutex> lock(_displayedSongListMutex);
    if (index >= this->_displayedSongList.size()) {
        return nullptr;
    }
    return this->_displayedSongList[index];
}

std::size_t BetterSongSearch::DataHolder::GetDisplayedSongListLength() {
    std::shared_lock<std::shared_mutex> lock(_displayedSongListMutex);
    return this->_displayedSongList.size();
}
