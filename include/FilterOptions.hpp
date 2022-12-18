#include "main.hpp"
#include<vector>
#pragma once

#include "main.hpp"

class FilterOptions
{
public:
    // FilterOptions(FilterOptions const&) = delete; // no accidental copying
    // FilterOptions() = default;

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

    enum class RequirementType {
        Any,
        NoodleExtensions,
        MappingExtensions,
        Chroma,
        Cinema
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
    int minUploadDate = BEATSAVER_EPOCH;
    float minRating = 0;
    int minVotes = 0;
    std::vector<std::string> uploaders;

    //Difficulty
    DifficultyFilterType difficultyFilter = DifficultyFilterType::All;
    CharFilterType charFilter = CharFilterType::All;

    //Mods
    RequirementType modRequirement = RequirementType::Any;
};

enum class SortMode {
    Newest,
    Oldest,
    Latest_Ranked,
    Most_Stars,
    Least_Stars,
    Best_rated,
    Worst_rated
};


class FilterOptionsCache
{
public:
    FilterOptionsCache(FilterOptionsCache const&) = delete; // no accidental copying
    FilterOptionsCache() = default;
   
    void cache(FilterOptions s){
        downloadType=s.downloadType;
        localScoreType=s.localScoreType;
        minLength=s.minLength;
        maxLength=s.maxLength;
        minNJS=s.minNJS;
        maxNJS=s.maxNJS;
        minNPS=s.minNPS;
        maxNPS=s.maxNPS;
        rankedType=s.rankedType;
        minStars=s.minStars;
        maxStars=s.maxStars;
        minUploadDate=s.minUploadDate;
        minRating=s.minRating;
        minVotes=s.minVotes;
        uploaders=s.uploaders;
        difficultyFilter=s.difficultyFilter;
        charFilter=s.charFilter;
        modRequirement=s.modRequirement;
    }
        

    //General
    FilterOptions::DownloadFilterType downloadType = FilterOptions::DownloadFilterType::All;
    FilterOptions::LocalScoreFilterType localScoreType = FilterOptions::LocalScoreFilterType::All;
    float minLength = 0, maxLength = 900;

    //Mapping
    float minNJS = 0, maxNJS = 25;
    float minNPS = 0, maxNPS = 12;

    //ScoreSaber
    FilterOptions::RankedFilterType rankedType = FilterOptions::RankedFilterType::All;
    float minStars = 0, maxStars = 14;

    //BeatSaver
    int minUploadDate = FilterOptions::BEATSAVER_EPOCH;
    float minRating = 0;
    int minVotes = 0;
    std::vector<std::string> uploaders;

    //Difficulty
    FilterOptions::DifficultyFilterType difficultyFilter = FilterOptions::DifficultyFilterType::All;
    FilterOptions::CharFilterType charFilter = FilterOptions::CharFilterType::All;

    //Mods
    FilterOptions::RequirementType modRequirement = FilterOptions::RequirementType::Any;
};
