#include "FilterOptions.hpp"

#include "Util/TextUtil.hpp"
#include "main.hpp"
#include "logging.hpp"

using namespace rapidjson;
using namespace UnityEngine;
using namespace BetterSongSearch::Util;


bool BetterSongSearch::FilterProfile::IsDefault(){
    return ( 
        downloadType == FilterTypes::DownloadFilter::All &&
        localScoreType ==  FilterTypes::LocalScoreFilter::All &&
        (maxLength / 60 >= SONG_LENGTH_FILTER_MAX) && 
        (minLength == 0) &&
        minNJS == 0 && 
        maxNJS >= NJS_FILTER_MAX &&
        minNPS == 0 &&
        maxNPS >= NPS_FILTER_MAX && 
        rankedType == FilterTypes::RankedFilter::ShowAll &&
        minStars == 0 &&
        maxStars >= STAR_FILTER_MAX &&
        minUploadDateInMonths == 0 &&
        minRating == 0 &&
        minVotes == 0 &&
        uploaders.empty() &&
        difficultyFilter == FilterTypes::DifficultyFilter::All &&
        charFilter == FilterTypes::CharFilter::All &&
        modRequirement == FilterTypes::Requirement::Any
    );
}

void BetterSongSearch::FilterProfile::LoadFromConfig() {
    downloadType = (FilterTypes::DownloadFilter) getPluginConfig().DownloadType.GetValue();
    localScoreType = (FilterTypes::LocalScoreFilter) getPluginConfig().LocalScoreType.GetValue();
    minLength = getPluginConfig().MinLength.GetValue();
    maxLength = getPluginConfig().MaxLength.GetValue();
    minNJS = getPluginConfig().MinNJS.GetValue();
    maxNJS = getPluginConfig().MaxNJS.GetValue();
    minNPS = getPluginConfig().MinNPS.GetValue();
    maxNPS = getPluginConfig().MaxNPS.GetValue();
    rankedType = (FilterTypes::RankedFilter) getPluginConfig().RankedType.GetValue();
    minStars = getPluginConfig().MinStars.GetValue();
    maxStars = getPluginConfig().MaxStars.GetValue();
    minUploadDate = getPluginConfig().MinUploadDate.GetValue();
    minRating = getPluginConfig().MinRating.GetValue();
    minVotes = getPluginConfig().MinVotes.GetValue();
    charFilter = (FilterTypes::CharFilter) getPluginConfig().CharacteristicType.GetValue();
    difficultyFilter = (FilterTypes::DifficultyFilter) getPluginConfig().DifficultyType.GetValue();
    modRequirement = (FilterTypes::Requirement) getPluginConfig().RequirementType.GetValue();
    minUploadDateInMonths = getPluginConfig().MinUploadDateInMonths.GetValue();

    // Custom string loader
    auto uploadersString = getPluginConfig().Uploaders.GetValue();
    if (!uploadersString.empty()) {
        if (uploadersString[0] == '!') {
            uploadersString.erase(0,1);
            uploadersBlackList = true;
        } else {
            uploadersBlackList = false;
        }
        uploaders = split(toLower(uploadersString), " ");
    } else {
        uploaders.clear();
    }
}

void BetterSongSearch::FilterProfile::SaveToConfig() {
    getPluginConfig().DownloadType.SetValue((int) downloadType, false);
    getPluginConfig().LocalScoreType.SetValue((int) localScoreType, false);
    getPluginConfig().MinLength.SetValue(minLength, false);
    getPluginConfig().MaxLength.SetValue(maxLength, false);
    getPluginConfig().MinNJS.SetValue(minNJS, false);
    getPluginConfig().MaxNJS.SetValue(maxNJS, false);
    getPluginConfig().MinNPS.SetValue(minNPS, false);
    getPluginConfig().MaxNPS.SetValue(maxNPS, false);
    getPluginConfig().RankedType.SetValue((int) rankedType, false);
    getPluginConfig().MinStars.SetValue(minStars, false);
    getPluginConfig().MaxStars.SetValue(maxStars, false);
    getPluginConfig().MinUploadDate.SetValue(minUploadDate, false);
    getPluginConfig().MinRating.SetValue(minRating, false);
    getPluginConfig().MinVotes.SetValue(minVotes, false);
    getPluginConfig().CharacteristicType.SetValue((int) charFilter, false);
    getPluginConfig().DifficultyType.SetValue((int) difficultyFilter, false);
    getPluginConfig().RequirementType.SetValue((int) modRequirement, false);
    getPluginConfig().MinUploadDateInMonths.SetValue(minUploadDateInMonths, false);

    getPluginConfig().Uploaders.SetValue((uploadersBlackList ? "!" : "") + join(uploaders, " "), false);

    getPluginConfig().Save();
}

std::optional<BetterSongSearch::FilterProfile> BetterSongSearch::FilterProfile::LoadFromPreset(std::string presetName){
    std::string presetsDir = getDataDir(modInfo) + "/Presets/";

    // Ensure the directory exists
    if (!direxists(presetsDir)) {
        mkpath(presetsDir);
    }

    std::string path = presetsDir + presetName + ".json";

    if (!fileexists(path)) {
        return std::nullopt;
    }

    try {
        return ReadFromFile<FilterProfile>(path);
    } catch (const std::exception &e) {
        ERROR("Failed to load preset: {}", e.what());
        return std::nullopt;
    }
}

bool BetterSongSearch::FilterProfile::SaveToPreset(std::string presetName) const {
    std::string presetsDir = getDataDir(modInfo) + "/Presets/";

    // Ensure the directory exists
    if (!direxists(presetsDir)) {
        mkpath(presetsDir);
    }

    std::string path = presetsDir + presetName + ".json";

    try {
        WriteToFile<FilterProfile>(path, *this);
        return true;
    } catch (const std::exception &e) {
        ERROR("Failed to save preset: {}", e.what());
        return false;
    }
}

std::vector<std::string> BetterSongSearch::FilterProfile::GetPresetList() {
    std::vector<std::string> presetNames;

    std::string presetsDir = getDataDir(modInfo) + "/Presets/";

    // Ensure the directory exists
    if (!direxists(presetsDir)) {
        mkpath(presetsDir);
    }

    if(!std::filesystem::is_directory(presetsDir)) return presetNames;

    std::error_code ec;
    auto directory_iterator = std::filesystem::directory_iterator(presetsDir, std::filesystem::directory_options::none, ec);
    for (auto const& entry : directory_iterator) {
        if(!entry.is_regular_file()) continue;
        std::string file_extension = entry.path().extension().string();
        std::string raw_file_name = entry.path().filename().replace_extension().string();
        if (file_extension == ".json") presetNames.push_back(raw_file_name);
    }
    std::sort(presetNames.begin(), presetNames.end(), [](std::string& a, std::string& b) {
        return a < b;
    });

    return presetNames;
}

void BetterSongSearch::FilterProfile::RecalculatePreprocessedValues(){
    charFilterPreprocessed = CHARACTERISTIC_MAP.at(charFilter);
    difficultyFilterPreprocessed = DIFFICULTY_MAP.at(difficultyFilter);
    isDefaultPreprocessed = IsDefault();
}



bool BetterSongSearch::FilterProfile::DeletePreset(std::string presetName) {
    std::string presetsDir = getDataDir(modInfo) + "/Presets/";

    // Ensure the directory exists
    if (!direxists(presetsDir)) {
        mkpath(presetsDir);
    }

    std::string path = presetsDir + presetName + ".json";

    if (!fileexists(path)) {
        return false;
    }

    try {
        std::filesystem::remove(path);
        return true;
    } catch (const std::exception &e) {
        ERROR("Failed to delete preset: {}", e.what());
        return false;
    }
}