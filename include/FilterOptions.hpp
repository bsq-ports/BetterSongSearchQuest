#pragma once

#include <vector>
#include <limits>

#include "rapidjson-macros/shared/macros.hpp"
#include "song-details/shared/Data/MapMods.hpp"
#include "PluginConfig.hpp"

namespace BetterSongSearch {
    DECLARE_JSON_CLASS(FilterProfile,
        VALUE_DEFAULT(FilterTypes::DownloadFilter, downloadType, FilterTypes::DownloadFilter::All);
        VALUE_DEFAULT(FilterTypes::LocalScoreFilter, localScoreType, FilterTypes::LocalScoreFilter::All);
        VALUE_DEFAULT(FilterTypes::RankedFilter, rankedType, FilterTypes::RankedFilter::ShowAll);
        VALUE_DEFAULT(FilterTypes::DifficultyFilter, difficultyFilter, FilterTypes::DifficultyFilter::All);
        VALUE_DEFAULT(FilterTypes::CharFilter, charFilter, FilterTypes::CharFilter::All);
        VALUE_DEFAULT(FilterTypes::Requirement, modRequirement, FilterTypes::Requirement::Any);

        VALUE_DEFAULT(float, minLength, 0);
        VALUE_DEFAULT(float, maxLength, 900);
        VALUE_DEFAULT(float, minNJS, 0);
        VALUE_DEFAULT(float, maxNJS, NJS_FILTER_MAX);
        VALUE_DEFAULT(float, minNPS, 0);
        VALUE_DEFAULT(float, maxNPS, NPS_FILTER_MAX);
        
        VALUE_DEFAULT(float, minStars, 0);
        VALUE_DEFAULT(float, maxStars, STAR_FILTER_MAX);
        VALUE_DEFAULT(int, minUploadDate, BEATSAVER_EPOCH);
        VALUE_DEFAULT(int, minUploadDateInMonths, 0);
        VALUE_DEFAULT(float, minRating, 0);
        VALUE_DEFAULT(int, minVotes, 0);
        
        VECTOR_DEFAULT(std::string, uploaders, {});
        VALUE_DEFAULT(bool, uploadersBlackList, false);

        public:
            SongDetailsCache::MapCharacteristic charFilterPreprocessed = SongDetailsCache::MapCharacteristic::Custom;
            SongDetailsCache::MapDifficulty difficultyFilterPreprocessed = SongDetailsCache::MapDifficulty::Easy;

            bool isDefaultPreprocessed = true;

            // @brief Checks if the profile is the default profile (no filters)
            bool IsDefault();

            // @brief Recalculates preprocessed values
            void RecalculatePreprocessedValues();

            // @brief Loads the profile from the mod config
            void LoadFromConfig();

            // @brief Saves the profile to the mod config
            void SaveToConfig();

            // @brief Saves the profile to a preset
            // @param presetName The name of the preset to save
            // @return True if the preset was saved successfully
            bool SaveToPreset(std::string presetName) const;

            // @brief Deletes a preset
            // @param presetName The name of the preset to delete
            // @return True if the preset was deleted successfully
            static bool DeletePreset(std::string presetName);

            // @brief Gets a list of all available presets
            // @return A list of all available presets
            static std::vector<std::string> GetPresetList();

            // @brief Loads the profile from a preset
            // @param presetName The name of the preset to load
            // @return The loaded profile
            static std::optional<FilterProfile> LoadFromPreset(std::string presetName);
    )
}

