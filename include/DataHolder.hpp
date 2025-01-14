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
            // Song details ref
            SongDetailsCache::SongDetails* songDetails;

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

            // State variables to be globally accessible
            FilterTypes::SortMode sort = (FilterTypes::SortMode) 0; // UI sort state
            FilterTypes::SortMode currentSort = FilterTypes::SortMode::Newest; // Current search sort state
            std::string search = ""; // UI search string
            std::string currentSearch = ""; // Current search string

            

            FilterProfile filterOptions; // Filter options tied to ui
            FilterProfile filterOptionsCache; // Filter options for the current search

            /// @brief Player data model to get the scores (can easily be null)
            UnityW<GlobalNamespace::PlayerDataModel> playerDataModel = nullptr;

            // Preferred Leaderboard
            FilterTypes::PreferredLeaderBoard preferredLeaderboard = FilterTypes::PreferredLeaderBoard::ScoreSaber;

            
            bool loaded = false; // Song data is loaded
            bool failed = false; // Song data failed to load 
            bool loading = false; // Song data is loading
            bool needsRefresh = false;  // Song list needs to be refreshed when shown
            bool searchInProgress = false; // Search in progress (unused for now)

            /// @brief Initializes the data holder and starts loading the song data and subscribing to the events
            void Init();

            /// @brief Downloads the song list
            void DownloadSongList();
            
            /// @brief Preprocesses the tags
            void PreprocessTags();

            /// @brief Updates the player scores
            void UpdatePlayerScores();

            /// @brief Checks if a song is in the list of songs with scores
            bool SongHasScore(const SongDetailsCache::Song* song);
            bool SongHasScore(std::string_view songhash);
            void Search();
        private:
            std::shared_mutex mutex_songsWithScores; // Mutex for the songs with scores set
            std::unordered_set<std::string> songsWithScores; // Songs with scores (hashes) for played songs filtering

            /// @brief Called when the song data is done loading
            void SongDataDone();
            /// @brief Called when the song data failed to load
            void SongDataError(std::string message);
    };


    // Instance of the data holder
    inline static DataHolder dataHolder;
}