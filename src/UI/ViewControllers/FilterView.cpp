#include "UI/ViewControllers/FilterView.hpp"
#include "main.hpp"
#include "PluginConfig.hpp"
#include "assets.hpp"
#include "bsml/shared/BSML.hpp"
#include "FilterOptions.hpp"
#include "DateUtils.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include <fmt/chrono.h>
#include "Util/BSMLStuff.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"


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
    
    SAVE_NUMBER_CONFIG(this->minimumSongLength,MinLength ,minLength);
    SAVE_NUMBER_CONFIG(this->maximumSongLength, MaxLength,maxLength);
    SAVE_NUMBER_CONFIG(this->minimumNjs, MinNJS, minNJS);
    SAVE_NUMBER_CONFIG(this->maximumNjs,MaxNJS,  maxNJS);
    SAVE_NUMBER_CONFIG(this->minimumNps, MinNPS, minNPS);
    SAVE_NUMBER_CONFIG(this->maximumNps,MaxNPS,  maxNPS);
    SAVE_NUMBER_CONFIG(this->minimumStars,MinStars, minStars);
    SAVE_NUMBER_CONFIG(this->maximumStars,MaxStars,  maxStars);
    SAVE_NUMBER_CONFIG(this->minimumRating, MinRating, minRating);
    SAVE_INTEGER_CONFIG(this->minimumVotes,MinVotes, minVotes);
    SAVE_INTEGER_CONFIG(this->hideOlderThan, MinUploadDateInMonths, minUploadDate);

    if (filtersChanged) {
        DEBUG("Filters changed");
        auto controller = fcInstance->SongListController;
        controller->filterChanged = true;
        controller->SortAndFilterSongs(controller->prevSort, controller->prevSearch, true);
    } else {
        DEBUG("Filters did not change");
    }


}



void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    INFO("Filter View contoller activated");

    // Get settings and set stuff
    this->existingSongs=this->get_downloadedFilterOptions()->get_Item((int) DataHolder::filterOptions.downloadType);
    this->existingScore=this->get_scoreFilterOptions()->get_Item((int) DataHolder::filterOptions.localScoreType);
    this->minimumSongLength=DataHolder::filterOptions.minLength;
    this->maximumSongLength=DataHolder::filterOptions.maxLength;
    this->minimumNjs = DataHolder::filterOptions.minNJS;
    this->maximumNjs = DataHolder::filterOptions.maxNJS;
    this->minimumNps = DataHolder::filterOptions.minNPS;
    this->maximumNps = DataHolder::filterOptions.maxNPS;
    this->minimumStars = DataHolder::filterOptions.minStars;
    this->maximumStars = DataHolder::filterOptions.maxStars;
    this->minimumRating = DataHolder::filterOptions.minRating;
    this->minimumVotes = DataHolder::filterOptions.minVotes;
    this->hideOlderThan = DataHolder::filterOptions.minUploadDate;
    
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
    hideOlderThanSlider->ReceiveValue();
    
    limitedUpdateFilterSettings = new BetterSongSearch::Util::RatelimitCoroutine([this]()
        { 
            coro(this->_UpdateFilterSettings());
        }, 0.2f);

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