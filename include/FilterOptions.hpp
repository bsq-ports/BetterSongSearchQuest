#include "main.hpp"
#pragma once

class FilterOptions
{
    public:
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
    const float SONG_LENGTH_FILTER_MAX = 15.0f;
	const float STAR_FILTER_MAX = 14.0f;
	const float NJS_FILTER_MAX = 25.0f;
	const float NPS_FILTER_MAX = 12.0f;

    //General
    DownloadFilterType downloadType = DownloadFilterType::All;
    LocalScoreFilterType localScoreType = LocalScoreFilterType::All;
    float minLength = 0, maxLength = 900;

    //Mapping
    float minNJS = 0, maxNJS = 25;
    float minNPS = 0, maxNPS = 12;

    //ScoreSaber
    bool showRanked = true;
    float minStars = 0, maxStars = 14;

    //BeatSaver
    float minimumRating = 0;
    int minimumVotes = 0;
};