#include "UI/ViewControllers/FilterView.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Backgroundable.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "HMUI/ImageView.hpp"

#include "main.hpp"
#include "PluginConfig.hpp"
#include "assets.hpp"

#include <fmt/chrono.h>

#include "FilterOptions.hpp"
#include "DateUtils.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "Util/BSMLStuff.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "Util/TextUtil.hpp"

using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI;

static const std::chrono::system_clock::time_point BEATSAVER_EPOCH_TIME_POINT{std::chrono::seconds(FilterOptions::BEATSAVER_EPOCH)};
DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))
#define SAVE_STRING_CONFIG(value, options, configName, filterProperty ) \
    if (value != nullptr) { \
        int index = get_##options()->IndexOf(value); \
        if (index < 0 ) { \
            ERROR("WE HAVE A BUG WITH SAVING VALUE {}", (std::string) value); \
        } else { \
            if (index != getPluginConfig().configName.GetValue()) { \
                filtersChanged = true; \
                getPluginConfig().configName.SetValue(index); \
                DataHolder::filterOptions.filterProperty = (typeof(DataHolder::filterOptions.filterProperty)) index; \
            } \
        }\
    }

#define SAVE_NUMBER_CONFIG(value, configName, filterProperty) \
    if (value != getPluginConfig().configName.GetValue()) { \
        filtersChanged = true; \
        getPluginConfig().configName.SetValue(value); \
        DataHolder::filterOptions.filterProperty = (typeof(DataHolder::filterOptions.filterProperty)) value; \
    } \

// TODO: Fix saving last saved
#define SAVE_INTEGER_CONFIG(value, configName, filterProperty) \
    if (static_cast<int>(value) != getPluginConfig().configName.GetValue()) { \
        filtersChanged = true; \
        getPluginConfig().configName.SetValue(static_cast<int>(value)); \
        DataHolder::filterOptions.filterProperty = static_cast<int>(value); \
    } \


// TODO: Fix unlimited to better search songs outside of filters boundaries
custom_types::Helpers::Coroutine ViewControllers::FilterViewController::_UpdateFilterSettings() {
    // Wait for next frame
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.1f));

    // WARNING: There is a bug with bsml update, it runs before the value is changed for some reason
    bool filtersChanged = false;

    SAVE_STRING_CONFIG(this->existingSongs, downloadedFilterOptions, DownloadType, downloadType);
    SAVE_STRING_CONFIG(this->existingScore, scoreFilterOptions, LocalScoreType , localScoreType);

    SAVE_STRING_CONFIG(this->characteristic, characteristics, CharacteristicType, charFilter);
    SAVE_STRING_CONFIG(this->difficulty, difficulties, DifficultyType, difficultyFilter);
    SAVE_STRING_CONFIG(this->rankedState, rankedFilterOptions, RankedType, rankedType);
    SAVE_STRING_CONFIG(this->mods, modOptions, RequirementType, modRequirement);
    SAVE_NUMBER_CONFIG(this->minimumNjs, MinNJS, minNJS);
    SAVE_NUMBER_CONFIG(this->maximumNjs,MaxNJS,  maxNJS);
    SAVE_NUMBER_CONFIG(this->minimumNps, MinNPS, minNPS);
    SAVE_NUMBER_CONFIG(this->maximumNps,MaxNPS,  maxNPS);
    SAVE_NUMBER_CONFIG(this->minimumStars,MinStars, minStars);
    SAVE_NUMBER_CONFIG(this->maximumStars,MaxStars,  maxStars);
    SAVE_NUMBER_CONFIG(this->minimumRating, MinRating, minRating);
    SAVE_INTEGER_CONFIG(this->minimumVotes,MinVotes, minVotes);

    // Special case for saving date
    if (this->hideOlderThan != getPluginConfig().MinUploadDateInMonths.GetValue()) {
        filtersChanged = true;
        std::chrono::time_point val = BEATSAVER_EPOCH_TIME_POINT + std::chrono::months((int)this->hideOlderThan);
        int sec = duration_cast<std::chrono::seconds>(val.time_since_epoch()).count();

        DataHolder::filterOptions.minUploadDate = sec;
        getPluginConfig().MinUploadDate.SetValue(sec);
        getPluginConfig().MinUploadDateInMonths.SetValue(this->hideOlderThan);
    }

    // Special case for min song length
    if (this->minimumSongLength * 60 != getPluginConfig().MinLength.GetValue()) {
        int seconds = minimumSongLength * 60;

        filtersChanged = true;
        DataHolder::filterOptions.minLength = seconds;
        getPluginConfig().MinLength.SetValue(seconds);
    }

    // Special case for max song length
    if (this->maximumSongLength * 60 != getPluginConfig().MaxLength.GetValue()) {
        int seconds = maximumSongLength * 60;

        filtersChanged = true;
        DataHolder::filterOptions.maxLength = seconds;
        getPluginConfig().MaxLength.SetValue(seconds);
    }

    // Save uploaders
    if (this->uploadersString != getPluginConfig().Uploaders.GetValue()) {
        filtersChanged = true;

        // Save to config
        getPluginConfig().Uploaders.SetValue(this->uploadersString);

        // Apply to filters
        std::string copy = uploadersString;
        if (copy.size() > 0) {
            if (copy[0] == '!') {
                copy.erase(0,1);
                DataHolder::filterOptions.uploadersBlackList = true;
            } else {
                DataHolder::filterOptions.uploadersBlackList = false;
            }
            DataHolder::filterOptions.uploaders = split(toLower(copy), " ");
        } else {
            DataHolder::filterOptions.uploaders.clear();
        }
    }
    
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


    if (filtersChanged) {
        DEBUG("Filters changed");
        auto controller = fcInstance->SongListController;
        controller->filterChanged = true;
        controller->SortAndFilterSongs(controller->prevSort, controller->prevSearch, true);
    } else {
        DEBUG("Filters did not change");
    }


}

UnityEngine::Sprite* GetBGSprite(std::string str)
{
    return QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>(),
                                     [str](UnityEngine::Sprite* x) {
                                         return to_utf8(csstrtostr(x->get_name())) == str;
                                     });
}

void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    // It needs to be registered
    limitedUpdateFilterSettings = new BetterSongSearch::Util::RatelimitCoroutine([this]()
    {
        DEBUG("RUNNING update");
        coro(this->_UpdateFilterSettings());
    }, 0.2f);

    INFO("Filter View contoller activated");

    // Get settings and set stuff
    this->existingSongs=this->get_downloadedFilterOptions()->get_Item((int) DataHolder::filterOptions.downloadType);
    this->existingScore=this->get_scoreFilterOptions()->get_Item((int) DataHolder::filterOptions.localScoreType);
    this->minimumSongLength=DataHolder::filterOptions.minLength / 60.0f;
    this->maximumSongLength=DataHolder::filterOptions.maxLength / 60.0f;
    this->minimumNjs = DataHolder::filterOptions.minNJS;
    this->maximumNjs = DataHolder::filterOptions.maxNJS;
    this->minimumNps = DataHolder::filterOptions.minNPS;
    this->maximumNps = DataHolder::filterOptions.maxNPS;
    this->minimumStars = DataHolder::filterOptions.minStars;
    this->maximumStars = DataHolder::filterOptions.maxStars;
    this->minimumRating = DataHolder::filterOptions.minRating;
    this->minimumVotes = DataHolder::filterOptions.minVotes;
    this->hideOlderThan = getPluginConfig().MinUploadDateInMonths.GetValue();

    // Custom string loader
    this->uploadersString = getPluginConfig().Uploaders.GetValue();

    
    // TODO: fix uploaders field loading
    // this->uploadersString = StringW(DataHolder::filterOptions.uploaders);
    this->characteristic = this->get_characteristics()->get_Item((int) DataHolder::filterOptions.charFilter);
    this->difficulty = this->get_difficulties()->get_Item((int) DataHolder::filterOptions.difficultyFilter);
    this->rankedState = this->get_rankedFilterOptions()->get_Item((int) DataHolder::filterOptions.rankedType);
    this->mods =  this->get_modOptions()->get_Item((int) DataHolder::filterOptions.modRequirement);

    // Create bsml view
    BSML::parse_and_construct(IncludedAssets::FilterView_bsml, this->get_transform(), this);

    auto maxUploadDate = GetMonthsSinceDate(FilterOptions::BEATSAVER_EPOCH);

    coro(BetterSongSearch::UI::Util::BSMLStuff::MergeSliders(this->get_gameObject()));



    // Apply formatter functions Manually cause Red did not implement parsing for them in bsml
    std::function<StringW(float monthsSinceFirstUpload)>   DateTimeToStr = [](float monthsSinceFirstUpload)
    {
        auto val = BEATSAVER_EPOCH_TIME_POINT + std::chrono::months(int(monthsSinceFirstUpload));
        return fmt::format("{:%b:%Y}", fmt::localtime(val));
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

    for(auto x : QuestUI::ArrayUtil::Select<HMUI::ImageView*>(GetComponentsInChildren<BSML::Backgroundable*>(), [](BSML::Backgroundable* x) {return x->GetComponent<HMUI::ImageView*>();})) {
        if(!x || x->get_color0() != Color::get_white() || x->get_sprite()->get_name() != "RoundRect10")
            continue;
        x->skew = 0.0f;
        x->set_overrideSprite(nullptr);
        x->set_sprite(getBgSprite);
        x->set_color(Color(0.0f, 0.7f, 1.0f, 0.4f));
    }

    for(auto x : QuestUI::ArrayUtil::Where(filterbarContainer->GetComponentsInChildren<HMUI::ImageView*>(), [](HMUI::ImageView* x) {return x->get_gameObject()->get_name() == "Underline";}))
        x->set_sprite(getBgSprite);

    // Format other values
    std::function<std::string(float)> minLengthSliderFormatFunction = [](float value) {
        float totalSeconds = value * 60;
        int minutes = ((int)totalSeconds % 3600) / 60;
        int seconds = (int)totalSeconds % 60;

        return fmt::format("{:02}:{:02}", minutes, seconds);
        
    };
    minimumSongLengthSlider->formatter = minLengthSliderFormatFunction;

    // Max length format
    std::function<std::string(float)> maxLengthSliderFormatFunction = [](float value) {
        float totalSeconds = value * 60;
        int minutes = ((int)totalSeconds % 3600) / 60;
        int seconds = (int)totalSeconds % 60;

        if (value >= DataHolder::filterOptions.SONG_LENGTH_FILTER_MAX) {
            return (std::string) "Unlimited";
        } else {
            return fmt::format("{:02}:{:02}", minutes, seconds);
        }
    };
    maximumSongLengthSlider->formatter = maxLengthSliderFormatFunction;

    // Min rating format
    std::function<std::string(float)> minRatingSliderFormatFunction = [](float value) {
        return fmt::format("{:.1f}%", value*100);
    };
    minimumRatingSlider->formatter = minRatingSliderFormatFunction;

    // NJS format
    std::function<std::string(float)> minNJSFormat = [](float value) {
            return fmt::format("{:.1f}", value);
    };
    std::function<std::string(float)> maxNJSFormat = [](float value) {
        if (value >= DataHolder::filterOptions.NJS_FILTER_MAX) {
            return  (std::string)  "Unlimited";
        }
        return fmt::format("{:.1f}", value);
    };
    minimumNjsSlider->formatter = minNJSFormat;
    maximumNjsSlider->formatter = maxNJSFormat;
    
    // NPS format
    std::function<std::string(float)> minNPSFormat = [](float value) {
        return fmt::format("{:.1f}", value);
    };
    std::function<std::string(float)> maxNPSFormat = [](float value) {
        if (value >= DataHolder::filterOptions.NPS_FILTER_MAX) {
            return  (std::string)  "Unlimited";
        }
        return fmt::format("{:.1f}", value);
    };
    minimumNpsSlider->formatter = minNPSFormat;
    maximumNpsSlider->formatter = maxNPSFormat;

    // Stars formatting
    std::function<std::string(float)> minStarFormat = [](float value) {
        return fmt::format("{:.1f}", value);
    };
    std::function<std::string(float)> maxStarFormat = [](float value) {
        if (value >= DataHolder::filterOptions.STAR_FILTER_MAX) {
            return  (std::string)  "Unlimited";
        }
        return fmt::format("{:.1f}", value);
    };
    minStarsSetting->formatter = minStarFormat;
    maxStarsSetting->formatter = maxStarFormat;
    std::function<std::string(float)> minimumVotesFormat = [](float value) {
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
        }
        auto uploaders = split(value, " ");

        return fmt::format("{} <color=#CCC>{}</color> uploader", (blacklist ? "Hiding": "Show only"), uploaders.size(), (uploaders.size() == 1 ? "" : "s") );
    };

    uploadersStringControl->formatter = uploadersStringFormat;

    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/FilterView.bsml";
    #endif
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


// Top buttons
void ViewControllers::FilterViewController::ClearFilters()
{
    DEBUG("ClearFilters FIRED");
}
void ViewControllers::FilterViewController::ShowPresets()
{
    DEBUG("ShowPresets FIRED");
}



// StringW ViewControllers::FilterViewController::DateTimeToStr(int d) {
//     // FilterView.hideOlderThanOptions[d].ToString("MMM yyyy", CultureInfo.InvariantCulture);
// }