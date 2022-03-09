#include "main.hpp"
#include<vector>
#pragma once

#include "main.hpp"


#include "questui_components/shared/components/settings/DropdownSetting.hpp"

class FilterOptions
{
    public:
    FilterOptions(FilterOptions const&) = delete; // no accidental copying
    FilterOptions() = default;

    enum class DownloadFilterType
    {
        All,
        OnlyDownloaded,
        HideDownloaded
    };
    enum class LocalScoreFilterType
    {
        All,
        HidePassed,
        OnlyPassed
    };
    enum class RankedFilterType
    {
        All,
        HideRanked,
        OnlyRanked
    };
    enum class DifficultyFilterType
    {
        All,
        Easy,
        Normal,
        Hard,
        Expert,
        ExpertPlus
    };
    enum class CharFilterType
    {
        All,
        Custom,
        Standard,
        OneSaber,
        NoArrows,
        NinetyDegrees,
        ThreeSixtyDegrees,
        LightShow,
        Lawless,
    };
    const float SONG_LENGTH_FILTER_MAX = 15.0f;
	const float STAR_FILTER_MAX = 14.0f;
	const float NJS_FILTER_MAX = 25.0f;
	const float NPS_FILTER_MAX = 12.0f;
    static const int64_t BEATSAVER_EPOCH = 1525136400;

    //General
    DownloadFilterType downloadType = DownloadFilterType::All;
    LocalScoreFilterType localScoreType = LocalScoreFilterType::All;
    float minLength = 0, maxLength = 900;

    //Mapping
    float minNJS = 0, maxNJS = 25;
    float minNPS = 0, maxNPS = 12;

    //ScoreSaber
    RankedFilterType rankedType = RankedFilterType::All;
    float minStars = 0, maxStars = 14;

    //BeatSaver
    float minRating = 0;
    int minVotes = 0;
    std::vector<std::string> uploaders;

    //Difficulty
    DifficultyFilterType difficultyFilter = DifficultyFilterType::All;
    CharFilterType charFilter = CharFilterType::All;
};

DROPDOWN_CREATE_ENUM_CLASS(SortMode,
                           STR_LIST("Newest", "Oldest", "Latest Ranked", "Most Stars", "Least Stars", "Best rated", "Worst rated", "Most Downloads"),
                           Newest,
                           Oldest,
                           Latest_Ranked,
                           Most_Stars,
                           Least_Stars,
                           Best_rated,
                           Worst_rated,
                           Most_Downloads)


void SortAndFilterSongs(SortMode sort, std::string_view search);