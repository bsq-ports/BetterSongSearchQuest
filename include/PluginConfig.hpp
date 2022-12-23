#pragma once

#include "config-utils/shared/config-utils.hpp"

#include <string>
#include <unordered_map>

#include "FilterOptions.hpp"
#include "main.hpp"

DECLARE_CONFIG(PluginConfig,

CONFIG_VALUE(ReturnToBSS, bool, "Return to BSS from Solo", true);
CONFIG_VALUE(LoadSongPreviews, bool, "Load song previews", true);
CONFIG_VALUE(SmallerFontSize, bool, "Smaller font size", true);
CONFIG_VALUE(SortMode, int, "Sort Mode", 0);
CONFIG_VALUE(DownloadType, int, "Download Type", 0);
CONFIG_VALUE(LocalScoreType, int, "Local Score Type", 0);
CONFIG_VALUE(MinLength, float, "Minimum Song Length", 0);
CONFIG_VALUE(MaxLength, float, "Maximum Song Length", 900);
CONFIG_VALUE(MinNJS, float, "Minimum Note Jump Speed", 0);
CONFIG_VALUE(MaxNJS, float, "Maximum Note Jump Speed", 25);
CONFIG_VALUE(MinNPS, float, "Minimum Notes Per Second", 0);
CONFIG_VALUE(MaxNPS, float, "Maximum Note Per Second", 12);
CONFIG_VALUE(RankedType, int, "Ranked Type", 0);
CONFIG_VALUE(MinStars, float, "Minimum Ranked Stars", 0);
CONFIG_VALUE(MaxStars, float, "Maximum Ranked Stars", 14);
CONFIG_VALUE(MinUploadDateInMonths, int, "Minimum Upload Date In Months", 0); //TEMPORARY UNTIL I FIX CONVERTING UNIX -> MONTHS SINCE BEAT SAVER
CONFIG_VALUE(MinUploadDate, int, "Minimum Upload Date", FilterOptions::BEATSAVER_EPOCH);
CONFIG_VALUE(MinRating, float, "Minimum Rating", 0);
CONFIG_VALUE(MinVotes, int, "Minimum Votes", 0);
CONFIG_VALUE(DifficultyType, int, "Difficulty Type", 0);
CONFIG_VALUE(CharacteristicType, int, "Characteristic Type", 0);
CONFIG_VALUE(Uploaders, std::string, "Characteristic Type", "");
CONFIG_VALUE(RequirementType, int, "Requirement Type", 0);
    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(ReturnToBSS);
        CONFIG_INIT_VALUE(LoadSongPreviews);
        CONFIG_INIT_VALUE(SmallerFontSize);
        CONFIG_INIT_VALUE(SortMode);
        CONFIG_INIT_VALUE(DownloadType);
        CONFIG_INIT_VALUE(LocalScoreType);
        CONFIG_INIT_VALUE(MinLength);
        CONFIG_INIT_VALUE(MaxLength);
        CONFIG_INIT_VALUE(MinNJS);
        CONFIG_INIT_VALUE(MaxNJS);
        CONFIG_INIT_VALUE(MinNPS);
        CONFIG_INIT_VALUE(MaxNPS);
        CONFIG_INIT_VALUE(RankedType);
        CONFIG_INIT_VALUE(MinStars);
        CONFIG_INIT_VALUE(MaxStars);
        CONFIG_INIT_VALUE(MinUploadDateInMonths);
        CONFIG_INIT_VALUE(MinUploadDate);
        CONFIG_INIT_VALUE(MinRating);
        CONFIG_INIT_VALUE(MinVotes);
        CONFIG_INIT_VALUE(DifficultyType);
        CONFIG_INIT_VALUE(CharacteristicType);
        CONFIG_INIT_VALUE(Uploaders);
        CONFIG_INIT_VALUE(RequirementType);
    )
)