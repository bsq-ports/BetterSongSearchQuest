#include "UI/ViewControllers/FilterView.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Backgroundable.hpp"
#include "HMUI/ImageView.hpp"

#include "logging.hpp"
#include "PluginConfig.hpp"
#include "assets.hpp"

#include <fmt/chrono.h>
#include <UnityEngine/Resources.hpp>

#include "DateUtils.hpp"
#include "Util/BSMLStuff.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "Util/TextUtil.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "DataHolder.hpp"

using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::UI::Util::BSMLStuff;


DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

#define SAVE_STRING_CONFIG(value, options, configName ) \
    if (value != nullptr) { \
        int index = get_##options()->IndexOf(reinterpret_cast<System::String*> (value.convert())); \
        if (index < 0 ) { \
            ERROR("WE HAVE A BUG WITH SAVING VALUE {}", (std::string) value); \
        } else { \
            if (index != getPluginConfig().configName.GetValue()) { \
                filtersChanged = true; \
                getPluginConfig().configName.SetValue(index); \
            } \
        }\
    }

#define SAVE_NUMBER_CONFIG(value, configName) \
    if (value != getPluginConfig().configName.GetValue()) { \
        filtersChanged = true; \
        getPluginConfig().configName.SetValue(value); \
    } \

// TODO: Fix saving last saved
#define SAVE_INTEGER_CONFIG(value, configName) \
    if (static_cast<int>(value) != getPluginConfig().configName.GetValue()) { \
        filtersChanged = true; \
        getPluginConfig().configName.SetValue(static_cast<int>(value)); \
    } \


// TODO: Fix unlimited to better search songs outside of filters boundaries
custom_types::Helpers::Coroutine ViewControllers::FilterViewController::_UpdateFilterSettings() {
    // Wait for next frame
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.1f));

    // WARNING: There is a bug with bsml update, it runs before the value is changed for some reason
    bool filtersChanged = false;

    SAVE_STRING_CONFIG(this->existingSongs, downloadedFilterOptions, DownloadType);
    SAVE_STRING_CONFIG(this->existingScore, scoreFilterOptions, LocalScoreType);

    SAVE_STRING_CONFIG(this->characteristic, characteristics, CharacteristicType);
    SAVE_STRING_CONFIG(this->difficulty, difficulties, DifficultyType);
    SAVE_STRING_CONFIG(this->rankedState, rankedFilterOptions, RankedType);
    SAVE_STRING_CONFIG(this->mods, modOptions, RequirementType);
    SAVE_NUMBER_CONFIG(this->minimumNjs, MinNJS);
    SAVE_NUMBER_CONFIG(this->maximumNjs, MaxNJS);
    SAVE_NUMBER_CONFIG(this->minimumNps, MinNPS);
    SAVE_NUMBER_CONFIG(this->maximumNps, MaxNPS);
    SAVE_NUMBER_CONFIG(this->minimumStars, MinStars);
    SAVE_NUMBER_CONFIG(this->maximumStars, MaxStars);
    SAVE_NUMBER_CONFIG(this->minimumRating, MinRating);
    SAVE_INTEGER_CONFIG(this->minimumVotes, MinVotes);

    // Special case for saving date
    if (this->hideOlderThan != getPluginConfig().MinUploadDateInMonths.GetValue()) {
        filtersChanged = true;

        auto timestamp = GetDateAfterMonths(BEATSAVER_EPOCH, this->hideOlderThan).count();
        DEBUG("Date {}", GetDateAfterMonths(BEATSAVER_EPOCH, this->hideOlderThan));

        getPluginConfig().MinUploadDate.SetValue(timestamp);
        getPluginConfig().MinUploadDateInMonths.SetValue(this->hideOlderThan);
    }

    // Special case for min song length
    if (this->minimumSongLength * 60 != getPluginConfig().MinLength.GetValue()) {
        int seconds = minimumSongLength * 60;

        filtersChanged = true;
        getPluginConfig().MinLength.SetValue(seconds);
    }

    // Special case for max song length
    if (this->maximumSongLength * 60 != getPluginConfig().MaxLength.GetValue()) {
        int seconds = maximumSongLength * 60;

        filtersChanged = true;
        getPluginConfig().MaxLength.SetValue(seconds);
    }

    // Save uploaders
    if (this->uploadersString != getPluginConfig().Uploaders.GetValue()) {
        filtersChanged = true;

        // Save to config
        getPluginConfig().Uploaders.SetValue(this->uploadersString);
    }
    
    if (this->onlyCuratedMaps != getPluginConfig().OnlyCuratedMaps.GetValue()) {
        filtersChanged = true;
        getPluginConfig().OnlyCuratedMaps.SetValue(this->onlyCuratedMaps);
    }

    if (this->onlyVerifiedMappers != getPluginConfig().OnlyVerifiedMappers.GetValue()) {
        filtersChanged = true;
        getPluginConfig().OnlyVerifiedMappers.SetValue(this->onlyVerifiedMappers);
    }

    if (this->onlyV3Maps != getPluginConfig().OnlyV3Maps.GetValue()) {
        filtersChanged = true;
        getPluginConfig().OnlyV3Maps.SetValue(this->onlyV3Maps);
    }

    if (filtersChanged) {
        DEBUG("Filters changed");

        // Update filter options state
        dataHolder.filterOptions.LoadFromConfig();

        auto controller = fcInstance->SongListController;
        dataHolder.filterChanged = true;
        controller->SortAndFilterSongs(dataHolder.sort, dataHolder.search, true);
    } else {
        DEBUG("Filters did not change");
    }
}

UnityEngine::Sprite* GetBGSprite(std::string str)
{
    return UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>()->First([str](UnityEngine::Sprite* x) {
        return x->get_name() == str;
    });

}

void ViewControllers::FilterViewController::PostParse() {
    auto x = this->get_gameObject()->get_transform().cast<UnityEngine::RectTransform>();
    x->set_offsetMax(UnityEngine::Vector2(20.0f, 22.0f));

    auto maxUploadDate = BetterSongSearch::GetMonthsSinceDate(BEATSAVER_EPOCH);

    coro(BetterSongSearch::UI::Util::BSMLStuff::MergeSliders(this->get_gameObject()));

    // Apply formatter functions Manually cause Red did not implement parsing for them in bsml
    std::function<StringW(float monthsSinceFirstUpload)> DateTimeToStr = [](float monthsSinceFirstUpload)
    {
        auto val = BetterSongSearch::GetTimepointAfterMonths(BEATSAVER_EPOCH,monthsSinceFirstUpload);
        return fmt::format("{:%b:%Y}", fmt::localtime(std::chrono::system_clock::to_time_t(val)));
    };

    // Update the value and set the formatter
    hideOlderThanSlider->formatter = DateTimeToStr;
    hideOlderThanSlider->slider->set_maxValue(maxUploadDate);
    hideOlderThanSlider->slider->set_numberOfSteps(maxUploadDate);
    int steps = (hideOlderThanSlider->slider->get_maxValue() - hideOlderThanSlider->slider->get_minValue()) / hideOlderThanSlider->increments;
    hideOlderThanSlider->slider->set_numberOfSteps(steps + 1);
    hideOlderThanSlider->set_Value(getPluginConfig().MinUploadDateInMonths.GetValue());
    hideOlderThanSlider->ReceiveValue();


    auto getBgSprite = GetBGSprite("RoundRect10BorderFade");

    auto backgroundables = GetComponentsInChildren<BSML::Backgroundable*>();
    for (auto & backgroundable : backgroundables) {
        auto imageView = backgroundable->GetComponent<HMUI::ImageView*>();
        if (!imageView || !imageView->get_color0().Equals(UnityEngine::Color::get_white()) || imageView->get_sprite()->get_name() != "RoundRect10") {
            continue;
        }
        imageView->____skew = 0.0f;
        imageView->set_overrideSprite(nullptr);
        imageView->set_sprite(getBgSprite);
        imageView->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));
    }

    // Format other values
    std::function minLengthSliderFormatFunction = [](float value) {
        float totalSeconds = value * 60;
        int minutes = ((int)totalSeconds % 3600) / 60;
        int seconds = (int)totalSeconds % 60;

        return fmt::format("{:02}:{:02}", minutes, seconds);
        
    };
    minimumSongLengthSlider->formatter = minLengthSliderFormatFunction;

    // Max length format
    std::function maxLengthSliderFormatFunction = [](float value) {
        float totalSeconds = value * 60;
        int minutes = ((int)totalSeconds % 3600) / 60;
        int seconds = (int)totalSeconds % 60;

        if (value >= SONG_LENGTH_FILTER_MAX) {
            return (std::string) "Unlimited";
        } else {
            return fmt::format("{:02}:{:02}", minutes, seconds);
        }
    };
    maximumSongLengthSlider->formatter = maxLengthSliderFormatFunction;

    // Min rating format
    std::function minRatingSliderFormatFunction = [](float value) {
        return fmt::format("{:.1f}%", value*100);
    };
    minimumRatingSlider->formatter = minRatingSliderFormatFunction;

    // NJS format
    std::function minNJSFormat = [](float value) {
            return fmt::format("{:.1f}", value);
    };
    std::function maxNJSFormat = [](float value) {
        if (value >= NJS_FILTER_MAX) {
            return  (std::string)  "Unlimited";
        }
        return fmt::format("{:.1f}", value);
    };
    minimumNjsSlider->formatter = minNJSFormat;
    maximumNjsSlider->formatter = maxNJSFormat;
    
    // NPS format
    std::function minNPSFormat = [](float value) {
        return fmt::format("{:.1f}", value);
    };
    std::function maxNPSFormat = [](float value) {
        if (value >= NPS_FILTER_MAX) {
            return  (std::string)  "Unlimited";
        }
        return fmt::format("{:.1f}", value);
    };
    minimumNpsSlider->formatter = minNPSFormat;
    maximumNpsSlider->formatter = maxNPSFormat;

    // Stars formatting
    std::function minStarFormat = [](float value) {
        return fmt::format("{:.1f}", value);
    };
    std::function maxStarFormat = [](float value) {
        if (value >= STAR_FILTER_MAX) {
            return  (std::string)  "Unlimited";
        }
        return fmt::format("{:.1f}", value);
    };
    minStarsSetting->formatter = minStarFormat;
    maxStarsSetting->formatter = maxStarFormat;
    std::function minimumVotesFormat = [](float value) {
        return fmt::format("{}", (int) value);
    };
    minimumVotesSlider->formatter = minimumVotesFormat;

    std::function<std::string(StringW)> uploadersStringFormat = [](std::string value) {
        bool blacklist = false;
        if (value.size() > 0) {
            if (value[0] == '!') {
                value.erase(0,1);
                blacklist = true;
            }
        } else {
            return (std::string) "";
        }

        auto uploaders = split(value, " ");

        return fmt::format("{} <color=#CCC>{}</color> uploader", (blacklist ? "Hiding": "Show only"), uploaders.size(), (uploaders.size() == 1 ? "" : "s") );
    };

    uploadersStringControl->formatter = uploadersStringFormat;

    ForceFormatValues();
    
    // I hate BSML sometimes
    auto m = modsRequirementDropdown->dropdown->____modalView;
    m->get_transform().cast<UnityEngine::RectTransform>()->set_pivot(UnityEngine::Vector2(0.5f, 0.3f));
}

void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    // Register modals
    presetsModal = this->get_gameObject()->AddComponent<UI::Modals::Presets *>();
    genrePickerModal = this->get_gameObject()->AddComponent<UI::Modals::GenrePicker *>();

    // It needs to be registered
    limitedUpdateFilterSettings = new BetterSongSearch::Util::RatelimitCoroutine([this]()
    {
        DEBUG("RUNNING update");
        coro(this->_UpdateFilterSettings());
    }, 0.2f);

    INFO("Filter View controller activated");

    // Load the values from the config
    UpdateLocalState();

    // Create bsml view
    BSML::parse_and_construct(Assets::FilterView_bsml, this->get_transform(), this);

    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/bsml/BetterSongSearch/FilterView.bsml";
        fileWatcher->checkInterval = 0.5f;
    #endif
}

void ViewControllers::FilterViewController::ForceFormatValues() {
    // Force format values
    FormatSliderSettingValue(this->minStarsSetting);
    FormatSliderSettingValue(this->maxStarsSetting);
    FormatSliderSettingValue(this->minimumNjsSlider);
    FormatSliderSettingValue(this->maximumNjsSlider);
    FormatSliderSettingValue(this->minimumSongLengthSlider);
    FormatSliderSettingValue(this->hideOlderThanSlider);
    FormatSliderSettingValue(this->minimumNpsSlider);
    FormatSliderSettingValue(this->maximumNpsSlider);
    FormatSliderSettingValue(this->minimumRatingSlider);
    FormatSliderSettingValue(this->minimumVotesSlider);
    FormatStringSettingValue(this->uploadersStringControl);
}

void ViewControllers::FilterViewController::UpdateFilterSettings()
{
    // We need to wait 1 frame for all the properties to get applied and then save the values?
    coro(limitedUpdateFilterSettings->CallNextFrame());
}

// Sponsors related things
void ViewControllers::FilterViewController::OpenSponsorsModal()
{
    DEBUG("OpenSponsorsModal FIRED");
}
void ViewControllers::FilterViewController::CloseSponsorModal()
{
    DEBUG("CloseSponsorModal FIRED");
}
void ViewControllers::FilterViewController::OpenSponsorsLink()
{
    DEBUG("OpenSponsorsLink FIRED");
}
void ViewControllers::FilterViewController::ShowGenrePicker()
{
    DEBUG("ShowGenrePicker FIRED");
    this->genrePickerModal->OpenModal();
}



void ViewControllers::FilterViewController::UpdateLocalState() {
    // Load from dataHolder
    this->existingSongs=this->get_downloadedFilterOptions()->get_Item((int) dataHolder.filterOptions.downloadType);
    this->existingScore=this->get_scoreFilterOptions()->get_Item((int) dataHolder.filterOptions.localScoreType);
    this->characteristic = this->get_characteristics()->get_Item((int) dataHolder.filterOptions.charFilter);
    this->rankedState = this->get_rankedFilterOptions()->get_Item((int) dataHolder.filterOptions.rankedType);
    this->difficulty = this->get_difficulties()->get_Item((int) dataHolder.filterOptions.difficultyFilter);
    this->mods =  this->get_modOptions()->get_Item((int) dataHolder.filterOptions.modRequirement);

    this->minimumSongLength=dataHolder.filterOptions.minLength / 60.0f;
    this->maximumSongLength=dataHolder.filterOptions.maxLength / 60.0f;
    this->minimumNjs = dataHolder.filterOptions.minNJS;
    this->maximumNjs = dataHolder.filterOptions.maxNJS;
    this->minimumNps = dataHolder.filterOptions.minNPS;
    this->maximumNps = dataHolder.filterOptions.maxNPS;
    this->minimumStars = dataHolder.filterOptions.minStars;
    this->maximumStars = dataHolder.filterOptions.maxStars;
    this->minimumRating = dataHolder.filterOptions.minRating;
    this->minimumVotes = dataHolder.filterOptions.minVotes;

    // TODO: Maybe save it to the preset too
    this->hideOlderThan = getPluginConfig().MinUploadDateInMonths.GetValue();
    this->uploadersString = getPluginConfig().Uploaders.GetValue();

    this->onlyCuratedMaps = getPluginConfig().OnlyCuratedMaps.GetValue();
    this->onlyVerifiedMappers = getPluginConfig().OnlyVerifiedMappers.GetValue();
    this->onlyV3Maps = getPluginConfig().OnlyV3Maps.GetValue();
}

void ViewControllers::FilterViewController::ForceRefreshUI() {
    // Refresh UI
    // Force format values
    SetSliderSettingValue(this->minimumSongLengthSlider, this->minimumSongLength);
    SetSliderSettingValue(this->maximumSongLengthSlider, this->maximumSongLength);
    SetSliderSettingValue(this->minimumNjsSlider, this->minimumNjs);
    SetSliderSettingValue(this->maximumNjsSlider, this->maximumNjs);
    SetSliderSettingValue(this->minimumNpsSlider, this->minimumNps);
    SetSliderSettingValue(this->maximumNpsSlider, this->maximumNps);
    SetSliderSettingValue(this->minStarsSetting, this->minimumStars);
    SetSliderSettingValue(this->maxStarsSetting, this->maximumStars);
    SetSliderSettingValue(this->minimumRatingSlider, this->minimumRating);
    SetSliderSettingValue(this->minimumVotesSlider, this->minimumVotes);
    SetSliderSettingValue(this->hideOlderThanSlider, this->hideOlderThan);
    SetStringSettingValue(this->uploadersStringControl, getPluginConfig().Uploaders.GetValue());
    existingSongsSetting->set_Value(reinterpret_cast<System::String*> (this->existingSongs.convert()));
    existingScoreSetting->set_Value(reinterpret_cast<System::String*> (this->existingScore.convert()));
    rankedStateSetting->set_Value(reinterpret_cast<System::String*> (this->rankedState.convert()));
    characteristicDropdown->set_Value(reinterpret_cast<System::String*> (this->characteristic.convert()));
    difficultyDropdown->set_Value(reinterpret_cast<System::String*> (this->difficulty.convert()));
    modsRequirementDropdown->set_Value(reinterpret_cast<System::String*> (this->mods.convert()));
}

// Top buttons
void ViewControllers::FilterViewController::ClearFilters()
{
    DEBUG("ClearFilters FIRED");

    // Reset config
    getPluginConfig().DownloadType.SetValue(getPluginConfig().DownloadType.GetDefaultValue());
    getPluginConfig().LocalScoreType.SetValue(getPluginConfig().LocalScoreType.GetDefaultValue());
    getPluginConfig().CharacteristicType.SetValue(getPluginConfig().CharacteristicType.GetDefaultValue());
    getPluginConfig().RankedType.SetValue(getPluginConfig().RankedType.GetDefaultValue());
    getPluginConfig().DifficultyType.SetValue(getPluginConfig().DifficultyType.GetDefaultValue());
    getPluginConfig().RequirementType.SetValue(getPluginConfig().RequirementType.GetDefaultValue());
    getPluginConfig().MinLength.SetValue(getPluginConfig().MinLength.GetDefaultValue());
    getPluginConfig().MaxLength.SetValue(getPluginConfig().MaxLength.GetDefaultValue());
    getPluginConfig().MinNJS.SetValue(getPluginConfig().MinNJS.GetDefaultValue());
    getPluginConfig().MaxNJS.SetValue(getPluginConfig().MaxNJS.GetDefaultValue());
    getPluginConfig().MinNPS.SetValue(getPluginConfig().MinNPS.GetDefaultValue());
    getPluginConfig().MaxNPS.SetValue(getPluginConfig().MaxNPS.GetDefaultValue());
    getPluginConfig().MinStars.SetValue(getPluginConfig().MinStars.GetDefaultValue());
    getPluginConfig().MaxStars.SetValue(getPluginConfig().MaxStars.GetDefaultValue());
    getPluginConfig().MinUploadDate.SetValue(getPluginConfig().MinUploadDate.GetDefaultValue());
    getPluginConfig().MinRating.SetValue(getPluginConfig().MinRating.GetDefaultValue());
    getPluginConfig().MinVotes.SetValue(getPluginConfig().MinVotes.GetDefaultValue());
    getPluginConfig().Uploaders.SetValue(getPluginConfig().Uploaders.GetDefaultValue());
    getPluginConfig().MinUploadDateInMonths.SetValue(getPluginConfig().MinUploadDateInMonths.GetDefaultValue());
    getPluginConfig().MinUploadDate.SetValue(getPluginConfig().MinUploadDate.GetDefaultValue());
    getPluginConfig().OnlyVerifiedMappers.SetValue(getPluginConfig().OnlyVerifiedMappers.GetDefaultValue());
    getPluginConfig().OnlyCuratedMaps.SetValue(getPluginConfig().OnlyCuratedMaps.GetDefaultValue());
    getPluginConfig().OnlyV3Maps.SetValue(getPluginConfig().OnlyV3Maps.GetDefaultValue());
    getPluginConfig().MapGenreString.SetValue(getPluginConfig().MapGenreString.GetDefaultValue());
    getPluginConfig().MapStyleString.SetValue(getPluginConfig().MapStyleString.GetDefaultValue());
    getPluginConfig().MapGenreExcludeString.SetValue(getPluginConfig().MapGenreExcludeString.GetDefaultValue());

    // Load to dataHolder
    dataHolder.filterOptions.LoadFromConfig();

    // Refresh FilterView state from settings and DataHolder
    UpdateLocalState();

    // Force refresh UI
    ForceRefreshUI();

    DEBUG("Filters changed");
    auto controller = fcInstance->SongListController;
    dataHolder.filterChanged = true;
    controller->SortAndFilterSongs(dataHolder.sort, dataHolder.search, true);
}
void ViewControllers::FilterViewController::ShowPresets()
{
    this->presetsModal->OpenModal();
}

// StringW ViewControllers::FilterViewController::DateTimeToStr(int d) {
//     // FilterView.hideOlderThanOptions[d].ToString("MMM yyyy", CultureInfo.InvariantCulture);
// }
void ViewControllers::FilterViewController::TryToDownloadDataset()
{
    if (fcInstance) {
        if (fcInstance->SongListController) {
            fcInstance->SongListController->RetryDownloadSongList();
        }
    }
    DEBUG("TryToDownloadDataset");
}