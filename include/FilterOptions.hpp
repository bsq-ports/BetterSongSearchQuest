#pragma once

#include <vector>
#include <limits>
#include "main.hpp"
#include "song-details/shared/Data/MapCharacteristic.hpp"
#include "song-details/shared/Data/MapDifficulty.hpp"
#include "song-details/shared/Data/RankedStates.hpp"
#include "song-details/shared/Data/MapMods.hpp"
#include "song-details/shared/Data/SongDifficulty.hpp"



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
        ShowAll,
        ScoreSaberRanked,
        BeatLeaderRanked,
        ScoreSaberQualified,
        BeatLeaderQualified
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
        Cinema,
        None
    };

    static inline const float SONG_LENGTH_FILTER_MAX = 15.0f;
    static inline const float STAR_FILTER_MAX = 18.0f;
    static inline const float NJS_FILTER_MAX = 25.0f;
    static inline const float NPS_FILTER_MAX = 12.0f;
    static inline const int64_t BEATSAVER_EPOCH = 1525136400;

    //General
    DownloadFilterType downloadType = DownloadFilterType::All;
    LocalScoreFilterType localScoreType = LocalScoreFilterType::All;
    float minLength = 0, maxLength = 900;

    //Mapping
    float minNJS = 0, maxNJS = NJS_FILTER_MAX;
    float minNPS = 0, maxNPS = NPS_FILTER_MAX;

    // Ranked
    RankedFilterType rankedType = RankedFilterType::ShowAll;
    float minStars = 0, maxStars = STAR_FILTER_MAX;

    //BeatSaver
    int minUploadDate = BEATSAVER_EPOCH;
    int minUploadDateInMonths = 0;
    float minRating = 0;
    int minVotes = 0;
    std::vector<std::string> uploaders;
    bool uploadersBlackList = false;

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

enum class PreferredLeaderBoard {
    ScoreSaber = 0,
    BeatLeader = 1
};



// Map for characteristics
static const std::unordered_map<FilterOptions::CharFilterType, SongDetailsCache::MapCharacteristic> charMap = {
    {FilterOptions::CharFilterType::Custom, SongDetailsCache::MapCharacteristic::Custom},
    {FilterOptions::CharFilterType::Standard, SongDetailsCache::MapCharacteristic::Standard},
    {FilterOptions::CharFilterType::OneSaber, SongDetailsCache::MapCharacteristic::OneSaber},
    {FilterOptions::CharFilterType::NoArrows, SongDetailsCache::MapCharacteristic::NoArrows},
    {FilterOptions::CharFilterType::NinetyDegrees, SongDetailsCache::MapCharacteristic::NinetyDegree},
    {FilterOptions::CharFilterType::ThreeSixtyDegrees, SongDetailsCache::MapCharacteristic::ThreeSixtyDegree},
    {FilterOptions::CharFilterType::LightShow, SongDetailsCache::MapCharacteristic::LightShow},
    {FilterOptions::CharFilterType::Lawless, SongDetailsCache::MapCharacteristic::Lawless}
};

// Map for difficulties
static const std::unordered_map<FilterOptions::DifficultyFilterType, SongDetailsCache::MapDifficulty> diffMap = {
    {FilterOptions::DifficultyFilterType::Easy, SongDetailsCache::MapDifficulty::Easy},
    {FilterOptions::DifficultyFilterType::Normal, SongDetailsCache::MapDifficulty::Normal},
    {FilterOptions::DifficultyFilterType::Hard, SongDetailsCache::MapDifficulty::Hard},
    {FilterOptions::DifficultyFilterType::Expert, SongDetailsCache::MapDifficulty::Expert},
    {FilterOptions::DifficultyFilterType::ExpertPlus, SongDetailsCache::MapDifficulty::ExpertPlus}
};


// Map for ranked states
static const std::unordered_map<FilterOptions::RankedFilterType, SongDetailsCache::RankedStates> rankMap = {
    {FilterOptions::RankedFilterType::ScoreSaberRanked, SongDetailsCache::RankedStates::ScoresaberRanked},
    {FilterOptions::RankedFilterType::BeatLeaderRanked, SongDetailsCache::RankedStates::BeatleaderRanked},
    {FilterOptions::RankedFilterType::ScoreSaberQualified, SongDetailsCache::RankedStates::ScoresaberQualified},
    {FilterOptions::RankedFilterType::BeatLeaderQualified, SongDetailsCache::RankedStates::BeatleaderQualified}
};

// Map for preferred leaderboard
static const std::unordered_map<std::string, PreferredLeaderBoard> leaderBoardMap = {
    {"Scoresaber", PreferredLeaderBoard::ScoreSaber},
    {"Beatleader", PreferredLeaderBoard::BeatLeader}
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
        if (s.maxLength / 60 >= FilterOptions::SONG_LENGTH_FILTER_MAX) { maxLength=std::numeric_limits<float>::infinity(); } else { maxLength=s.maxLength;}
        minNJS=s.minNJS;
        if (s.maxNJS >= FilterOptions::NJS_FILTER_MAX) { maxNJS=std::numeric_limits<float>::infinity(); } else { maxNJS=s.maxNJS;}
        minNPS=s.minNPS;
        if (s.maxNPS >= FilterOptions::NPS_FILTER_MAX) { maxNPS=std::numeric_limits<float>::infinity(); } else { maxNPS=s.maxNPS;}
        rankedType=s.rankedType;
        minStars=s.minStars;
        if (s.maxStars >= FilterOptions::STAR_FILTER_MAX) { maxStars=std::numeric_limits<float>::infinity(); } else { maxStars=s.maxStars;}
        minUploadDate=s.minUploadDate;
        minRating=s.minRating;
        minVotes=s.minVotes;
        uploaders=s.uploaders;
        uploadersBlackList=s.uploadersBlackList;
        difficultyFilter=s.difficultyFilter;
        charFilter=s.charFilter;
        modRequirement=s.modRequirement;

        // Process char filter
        if (s.charFilter != FilterOptions::CharFilterType::All) {
            charFilterPreprocessed = charMap.at(s.charFilter);
        };

        // Map difficulty filter
        if (s.difficultyFilter != FilterOptions::DifficultyFilterType::All) {
            difficultyFilterPreprocessed = diffMap.at(s.difficultyFilter);
        };

        // Check if filter is needed at all
        skipFilter = false; 
        skipFilter = (
            downloadType == FilterOptions::DownloadFilterType::All &&
            localScoreType ==  FilterOptions::LocalScoreFilterType::All &&
            (s.maxLength / 60 >= FilterOptions::SONG_LENGTH_FILTER_MAX) && 
            (s.minLength == 0) &&
            minNJS == 0 && 
            s.maxNJS >= FilterOptions::NJS_FILTER_MAX &&
            s.minNPS == 0 &&
            s.maxNPS >= FilterOptions::NPS_FILTER_MAX && 
            rankedType == FilterOptions::RankedFilterType::ShowAll &&
            minStars == 0 &&
            s.maxStars >= FilterOptions::STAR_FILTER_MAX &&
            s.minUploadDateInMonths == 0 &&
            minRating == 0 &&
            minVotes == 0 &&
            uploaders.size() == 0 &&
            difficultyFilter == FilterOptions::DifficultyFilterType::All &&
            charFilter == FilterOptions::CharFilterType::All &&
            modRequirement == FilterOptions::RequirementType::Any
        );

        // Do infinity checks for songs that are out of bounds
        if (s.maxStars >= FilterOptions::STAR_FILTER_MAX) { maxStars=std::numeric_limits<float>::infinity(); }
        if (s.maxNJS >= FilterOptions::NJS_FILTER_MAX) { maxNJS=std::numeric_limits<float>::infinity(); }
        if (s.maxNPS >= FilterOptions::NPS_FILTER_MAX) { maxNPS=std::numeric_limits<float>::infinity(); }
        if (s.maxLength / 60 >= FilterOptions::SONG_LENGTH_FILTER_MAX) { maxLength=std::numeric_limits<float>::infinity(); }
    }

    bool skipFilter = false;
    

    //General
    FilterOptions::DownloadFilterType downloadType = FilterOptions::DownloadFilterType::All;
    FilterOptions::LocalScoreFilterType localScoreType = FilterOptions::LocalScoreFilterType::All;
    float minLength = 0, maxLength = 900;

    //Mapping
    float minNJS = 0, maxNJS = FilterOptions::NJS_FILTER_MAX;
    float minNPS = 0, maxNPS = FilterOptions::NPS_FILTER_MAX;

    //ScoreSaber
    FilterOptions::RankedFilterType rankedType = FilterOptions::RankedFilterType::ShowAll;
    float minStars = 0, maxStars = FilterOptions::STAR_FILTER_MAX;

    //BeatSaver
    int minUploadDate = FilterOptions::BEATSAVER_EPOCH;
    float minRating = 0;
    int minVotes = 0;
    std::vector<std::string> uploaders;
    bool uploadersBlackList = false;

    //Difficulty
    FilterOptions::DifficultyFilterType difficultyFilter = FilterOptions::DifficultyFilterType::All;
    SongDetailsCache::MapDifficulty difficultyFilterPreprocessed;


    /// @brief Char filter for gui
    FilterOptions::CharFilterType charFilter = FilterOptions::CharFilterType::All;
    /// @brief Used to speedup filtering, only if not All
    SongDetailsCache::MapCharacteristic charFilterPreprocessed = SongDetailsCache::MapCharacteristic::Custom;

    //Mods
    FilterOptions::RequirementType modRequirement = FilterOptions::RequirementType::Any;
};
