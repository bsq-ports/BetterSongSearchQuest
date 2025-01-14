#pragma once

#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"

#include "song-details/shared/Data/Song.hpp"
#include "song-details/shared/Data/SongDifficulty.hpp"
#include "song-details/shared/Data/MapCharacteristic.hpp"
#include "song-details/shared/SongDetails.hpp"

#include <vector>
#include <unordered_map>

#include "FilterOptions.hpp"

/*
    Global data holder for the mod to simplify access to global data
*/
namespace BetterSongSearch {
  
    struct PreprocessedTag {
        std::string tag;
        uint64_t mask;
        uint32_t songCount;
        bool isStyle;
    };

    // Global variables
    class DataHolder {
        public:
            SongDetailsCache::SongDetails* songDetails = nullptr; // Song details cache

            // Events
            UnorderedEventCallback<> loadingFinished; // Gets called when the loading is done
            UnorderedEventCallback<std::string> loadingFailed; // Gets called when the loading failed with the error message
            UnorderedEventCallback<> playerDataLoaded; // Callback when we process more player data

            std::vector<PreprocessedTag> tags = {}; // Preprocessed tags for filter UI
            std::unordered_map<std::string, uint64_t> tagMap = {};

            // Flag to say that the filter has changed and the song list needs to be updated, to be cleared after the update
            bool filterChanged = true;

            std::vector<const SongDetailsCache::Song*> filteredSongList; // Filtered songs
            std::vector<const SongDetailsCache::Song*> searchedSongList; // Searched songs
            std::vector<const SongDetailsCache::Song*> sortedSongList; // Sorted songs (actually displayed)

            FilterTypes::SortMode sort = (FilterTypes::SortMode) 0; // UI sort state
            FilterTypes::SortMode currentSort = FilterTypes::SortMode::Newest; // Current search sort state
            std::string search; // UI search string
            std::string currentSearch; // Current search string

            

            FilterProfile filterOptions; // Filter options tied to ui
            FilterProfile filterOptionsCache; // Filter options for the current search

            /// @brief Player data model to get the scores (can easily be null)
            UnityW<GlobalNamespace::PlayerDataModel> playerDataModel = nullptr;

            FilterTypes::PreferredLeaderBoard preferredLeaderboard = FilterTypes::PreferredLeaderBoard::ScoreSaber;

            bool loaded = false;
            bool failed = false;
            bool loading = false;
            bool needsRefresh = false;  // Song list needs to be refreshed when shown
            bool searchInProgress = false;

            /// @brief Initializes the data holder and starts loading the song data and subscribing to the events
            void Init();
            void DownloadSongList();
            void PreprocessTags();
            void UpdatePlayerScores();
            bool SongHasScore(const SongDetailsCache::Song* song);
            bool SongHasScore(std::string_view songhash);
            void Search();
        private:
            std::shared_mutex mutex_songsWithScores;
            std::unordered_set<std::string> songsWithScores; // Songs with scores (hashes) for played songs filtering

            void SongDataDone();
            void SongDataError(std::string message);
    };


    // Instance of the data holder
    inline static DataHolder dataHolder;
}