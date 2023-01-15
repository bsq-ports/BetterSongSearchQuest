#pragma once

#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "HMUI/ViewController.hpp"
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/ListSetting.hpp"
#include "bsml/shared/BSML/Components/Settings/StringSetting.hpp"
#include "bsml/shared/BSML/ViewControllers/HotReloadViewController.hpp"
#include "bsml/shared/BSML/Components/ClickableText.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"

#include "Util/RatelimitCoroutine.hpp"

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

#ifdef HotReload
DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, FilterViewController, BSML::HotReloadViewController,
#else
DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, FilterViewController, HMUI::ViewController,
#endif

    DECLARE_SIMPLE_DTOR();
    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);

    // Modal related things
    DECLARE_INSTANCE_METHOD(void, OpenSponsorsModal);
    DECLARE_INSTANCE_METHOD(void, CloseSponsorModal);
    DECLARE_INSTANCE_METHOD(void, OpenSponsorsLink);
    DECLARE_INSTANCE_METHOD(void, TryToDownloadDataset);
    

    // Header buttons
    DECLARE_INSTANCE_METHOD(void, ClearFilters);
    DECLARE_INSTANCE_METHOD(void, ShowPresets);


    // Options for dropdowns 
    BSML_OPTIONS_LIST_OBJECT(downloadedFilterOptions, "Show All", "Only Downloaded", "Hide Downloaded");
    BSML_OPTIONS_LIST_OBJECT(scoreFilterOptions, "Show All", "Hide Passed", "Only Passed");
    BSML_OPTIONS_LIST_OBJECT(rankedFilterOptions, "Show All", "Only Ranked", "Hide Ranked");
    BSML_OPTIONS_LIST_OBJECT(characteristics, "Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless");
    BSML_OPTIONS_LIST_OBJECT(difficulties, "Any", "Easy", "Normal", "Hard", "Expert", "Expert+");
    BSML_OPTIONS_LIST_OBJECT(modOptions, "Any", "Noodle Extensions", "Mapping Extensions", "Chroma", "Cinema", "None");

    // Values for dropdowns
    DECLARE_INSTANCE_FIELD(StringW, existingSongs);
    DECLARE_INSTANCE_FIELD(StringW, existingScore);
    DECLARE_INSTANCE_FIELD(StringW, rankedState);
    DECLARE_INSTANCE_FIELD(StringW, characteristic);
    DECLARE_INSTANCE_FIELD(StringW, difficulty);
    DECLARE_INSTANCE_FIELD(StringW, mods);

    // Values for sliders
    DECLARE_INSTANCE_FIELD(float, minimumSongLength);
    DECLARE_INSTANCE_FIELD(float, maximumSongLength);
    DECLARE_INSTANCE_FIELD(float, minimumNjs);
    DECLARE_INSTANCE_FIELD(float, maximumNjs);
    DECLARE_INSTANCE_FIELD(float, minimumNps);
    DECLARE_INSTANCE_FIELD(float, maximumNps);
    DECLARE_INSTANCE_FIELD(float, minimumStars);
    DECLARE_INSTANCE_FIELD(float, maximumStars);
    DECLARE_INSTANCE_FIELD(float, minimumRating);
    // These are float because there is some bug in 
    DECLARE_INSTANCE_FIELD(float, minimumVotes);
    DECLARE_INSTANCE_FIELD(float, hideOlderThan);

    DECLARE_INSTANCE_METHOD(void, UpdateFilterSettings);

    // BG layout stuff
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalLayoutGroup*, filterbarContainer);

    // Sliders that need to be formatted
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, hideOlderThanSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, minimumRatingSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, minStarsSetting);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, maxStarsSetting);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, minimumNjsSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, maximumNjsSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, minimumNpsSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, maximumNpsSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, minimumSongLengthSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, maximumSongLengthSlider);
    DECLARE_INSTANCE_FIELD(BSML::SliderSetting*, minimumVotesSlider);
    DECLARE_INSTANCE_FIELD(BSML::StringSetting*, uploadersStringControl);    

    // Values for string fields
    DECLARE_INSTANCE_FIELD(StringW, uploadersString);

    // Dropdowns and settings
    DECLARE_INSTANCE_FIELD(BSML::ListSetting*, existingSongsSetting);
    DECLARE_INSTANCE_FIELD(BSML::ListSetting*, existingScoreSetting);
    DECLARE_INSTANCE_FIELD(BSML::ListSetting*, rankedStateSetting);
    DECLARE_INSTANCE_FIELD(BSML::DropdownListSetting*, characteristicDropdown);
    DECLARE_INSTANCE_FIELD(BSML::DropdownListSetting*, difficultyDropdown);   
    DECLARE_INSTANCE_FIELD(BSML::DropdownListSetting*, modsRequirementDropdown);  

    DECLARE_INSTANCE_FIELD(BSML::ClickableText*, datasetInfoLabel);

    // DECLARE_INSTANCE_METHOD(StringW, minRatingSliderFormatFunction, float value);
    // DECLARE_INSTANCE_METHOD(StringW, minUploadDateSliderFormatFunciton, float monthsSinceFirstUpload);
    public:
        custom_types::Helpers::Coroutine _UpdateFilterSettings();
        BetterSongSearch::Util::RatelimitCoroutine* limitedUpdateFilterSettings = nullptr;
)