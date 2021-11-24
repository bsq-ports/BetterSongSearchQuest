#include "UI/ViewControllers/FilterView.hpp"
#include "main.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "questui/shared/CustomTypes/Components/Settings/SliderSetting.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
using namespace QuestUI;

#include "config-utils/shared/config-utils.hpp"

#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Resources.hpp"

#include "HMUI/CurvedCanvasSettingsHelper.hpp"
#include "HMUI/TimeSlider.hpp"
#include "HMUI/Touchable.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"

#include "DateUtils.hpp"
#include "FilterOptions.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "UI/SliderFormatter.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

UnityEngine::UI::VerticalLayoutGroup* filterViewLayout;

int getIndex(std::vector<std::string> v, std::string K)
{
    auto it = std::find(v.begin(), v.end(), K);
    if (it != v.end())
    {
        int index = it - v.begin();
        return index;
    }
    else {
        return -1;
    }
}

uint64_t timeSinceEpoch() {
  using namespace std::chrono;
  return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;
    get_rectTransform()->set_offsetMax(UnityEngine::Vector2(20, 22));
    get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(130);

    filterViewLayout = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    auto filterViewLayoutElement = filterViewLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    filterViewLayoutElement->set_preferredWidth(130);
    filterViewLayout->set_childControlHeight(false);
    filterViewLayout->get_gameObject()->AddComponent<HMUI::Touchable*>();
    filterViewLayout->get_rectTransform()->set_anchorMin(UnityEngine::Vector2(filterViewLayout->get_rectTransform()->get_anchorMin().x, 1));

    //Top Bar
    auto topBar = BeatSaberUI::CreateHorizontalLayoutGroup(filterViewLayout->get_transform());
    auto topBarElement = topBar->GetComponent<UnityEngine::UI::LayoutElement*>();
    topBarElement->set_preferredWidth(130);
    auto topBarBG = topBar->get_gameObject()->AddComponent<Backgroundable*>();
    topBarBG->ApplyBackgroundWithAlpha(il2cpp_utils::newcsstr("panel-top-gradient"), 1);
    auto imageView = (HMUI::ImageView*)topBarBG->background;
    imageView->skew = 0.18f;
    imageView->gradientDirection = HMUI::ImageView::GradientDirection::Vertical;
    imageView->set_color0(UnityEngine::Color(0.0f,0.75f, 1.0f, 1));
    imageView->set_color1(UnityEngine::Color(0.0f,0.37f, 0.5f, 1));
    imageView->gradient = true;
    imageView->set_material(QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>(), 
    [](UnityEngine::Material* x) { 
        return to_utf8(csstrtostr(x->get_name())) == "AnimatedButton"; 
    }));
    imageView->SetAllDirty();
    imageView->curvedCanvasSettingsHelper->Reset();
    std::function<void()> settingsButtonClick = [=]() {

    };
    std::function<void()> clearButtonClick = [=]() {

    };
    std::function<void()> presetsButtonClick = [=]() {

    };
    auto topBarSettingsButtonLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    topBarSettingsButtonLayout->set_padding(UnityEngine::RectOffset::New_ctor(5,0,0,0));
    auto topBarSettingsButton = BeatSaberUI::CreateUIButton(topBarSettingsButtonLayout->get_transform(), "Settings", UnityEngine::Vector2(0, 0), UnityEngine::Vector2(18, 7.74f), settingsButtonClick);
    BeatSaberUI::SetButtonTextSize(topBarSettingsButton, 4);
    auto topBarTitleLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    auto topBarTitleLayoutElement = topBarTitleLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    topBarTitleLayoutElement->set_preferredWidth(130);
    topBarTitleLayoutElement->set_ignoreLayout(true);
    auto topBarTitle = BeatSaberUI::CreateText(topBarTitleLayout->get_transform(), "FILTERS", true);
    topBarTitle->set_fontSize(7);
    topBarTitle->set_alignment(TMPro::TextAlignmentOptions::Center);
    auto topBarButtonsLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    auto topBarButtonsLayoutFitter = topBarButtonsLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    topBarButtonsLayoutFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    topBarButtonsLayout->set_spacing(2);
    topBarButtonsLayout->set_padding(UnityEngine::RectOffset::New_ctor(0,0,0,0));
    auto topBarClearButton = BeatSaberUI::CreateUIButton(topBarButtonsLayout->get_transform(), "Clear", UnityEngine::Vector2(0, 0), UnityEngine::Vector2(14, 7.74f), clearButtonClick);
    BeatSaberUI::SetButtonTextSize(topBarClearButton, 4);
    auto topBarPresetsButton = BeatSaberUI::CreateUIButton(topBarButtonsLayout->get_transform(), "Presets", UnityEngine::Vector2(0, 0), UnityEngine::Vector2(17, 7.74f), presetsButtonClick);
    BeatSaberUI::SetButtonTextSize(topBarPresetsButton, 4);
    
    //Filter Options
    auto filterOptionsLayout = BeatSaberUI::CreateHorizontalLayoutGroup(filterViewLayout->get_transform());
    auto filterOptionsLayoutElement = filterOptionsLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    auto filterOptionsLayoutFitter = filterOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    filterOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(0,0,3,1));
    filterOptionsLayoutElement->set_preferredHeight(80);
    filterOptionsLayout->set_childForceExpandWidth(true);
    filterOptionsLayoutFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

    //LeftSide
    auto leftOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(filterOptionsLayout->get_transform());
    auto leftOptionsLayoutElement = leftOptionsLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    leftOptionsLayout->set_childForceExpandHeight(false);
    leftOptionsLayout->set_childControlHeight(false);
    leftOptionsLayout->set_childScaleWidth(false);
    leftOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(0,2,0,0));
    leftOptionsLayoutElement->set_preferredWidth(65);

    auto generalTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    auto generalText = BeatSaberUI::CreateText(generalTextLayout->get_transform(), "[ General ]");
    generalText->set_fontSize(3.5);
    generalText->set_alignment(TMPro::TextAlignmentOptions::Center);
    generalText->set_fontStyle(TMPro::FontStyles::Underline);

    std::vector<std::string> downloadFilterOptions = {"Show all", "Only downloaded", "Hide downloaded"};
    std::function<void(std::string_view)> downloadFilterChange = [=](std::string_view value) {
        FilterOptions::DownloadFilterType type = FilterOptions::DownloadFilterType::All;
        filterOptions->downloadType = (FilterOptions::DownloadFilterType)getIndex(downloadFilterOptions, std::string(value));
        Sort();
    };
    auto downloadFilterDropdown = BeatSaberUI::CreateDropdown(leftOptionsLayout->get_transform(), "Downloaded", "Show all", downloadFilterOptions, downloadFilterChange);
    std::vector<std::string> scoreFilterOptions = {"Show all", "Hide passed", "Only passed"};
    std::function<void(std::string_view)> scoreFilterChange = [=](std::string_view value) {

    };
    auto scoreFilterDropdown = BeatSaberUI::CreateDropdown(leftOptionsLayout->get_transform(), "Local score", "Show all", scoreFilterOptions, scoreFilterChange);
    std::function<void(float)> minLengthChange = [=](float value) {

    };
    std::function<void(float)> maxLengthChange = [=](float value) {

    };

    auto lengthSliderLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    lengthSliderLayout->set_spacing(2);
    auto lengthSliders = BeatSaberUI::CreateHorizontalLayoutGroup(lengthSliderLayout->get_transform());
    lengthSliders->set_spacing(-2);
    auto lengthLabels = BeatSaberUI::CreateHorizontalLayoutGroup(lengthSliderLayout->get_transform());

    auto lengthLabel = BeatSaberUI::CreateText(lengthLabels->get_transform(), "Length");
    lengthLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

    auto minLengthSlider = BeatSaberUI::CreateSliderSetting(lengthSliderLayout->get_transform(), "", 0.25, 0, 0, 15, minLengthChange);
    auto maxLengthSlider = BeatSaberUI::CreateSliderSetting(lengthSliderLayout->get_transform(), "", 0.25, 0, 0, 15, maxLengthChange);

    reinterpret_cast<UnityEngine::RectTransform*>(minLengthSlider->slider->get_transform())->set_sizeDelta({20, 1});
    reinterpret_cast<UnityEngine::RectTransform*>(maxLengthSlider->slider->get_transform())->set_sizeDelta({20, 1});

    auto mappingTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    auto mappingText = BeatSaberUI::CreateText(mappingTextLayout->get_transform(), "[ Mapping ]");
    mappingText->set_fontSize(3.5);
    mappingText->set_alignment(TMPro::TextAlignmentOptions::Center);
    mappingText->set_fontStyle(TMPro::FontStyles::Underline);

    std::function<void(float)> minNJSChange = [=](float value) {
        filterOptions->minNJS = value;
        Sort();
    };
    std::function<void(float)> maxNJSChange = [=](float value) {
        filterOptions->maxNJS = value;
        Sort();
    };

    auto NJSSliderLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    NJSSliderLayout->set_spacing(2);
    auto NJSSliders = BeatSaberUI::CreateHorizontalLayoutGroup(NJSSliderLayout->get_transform());
    NJSSliders->set_spacing(-2);
    auto NJSLabels = BeatSaberUI::CreateHorizontalLayoutGroup(NJSSliderLayout->get_transform());

    auto NJSLabel = BeatSaberUI::CreateText(NJSLabels->get_transform(), "NJS");
    NJSLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

    auto minNJSSlider = BeatSaberUI::CreateSliderSetting(NJSSliderLayout->get_transform(), "", 0.5, 0, 0, 25, minNJSChange);
    auto maxNJSSlider = BeatSaberUI::CreateSliderSetting(NJSSliderLayout->get_transform(), "", 0.5, 0, 0, 25, maxNJSChange);

    reinterpret_cast<UnityEngine::RectTransform*>(minNJSSlider->slider->get_transform())->set_sizeDelta({20, 1});
    reinterpret_cast<UnityEngine::RectTransform*>(maxNJSSlider->slider->get_transform())->set_sizeDelta({20, 1});

    std::function<void(float)> minNPSChange = [=](float value) {
        filterOptions->minNPS = value;
        Sort();
    };
    std::function<void(float)> maxNPSChange = [=](float value) {
        filterOptions->maxNPS = value;
        Sort();
    };

    auto NPSSliderLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    NPSSliderLayout->set_spacing(2);
    auto NPSSliders = BeatSaberUI::CreateHorizontalLayoutGroup(NPSSliderLayout->get_transform());
    NPSSliders->set_spacing(-2);
    auto NPSLabels = BeatSaberUI::CreateHorizontalLayoutGroup(NPSSliderLayout->get_transform());

    auto NPSLabel = BeatSaberUI::CreateText(NPSLabels->get_transform(), "Notes/s");
    NPSLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

    auto minNPSSlider = BeatSaberUI::CreateSliderSetting(NPSSliderLayout->get_transform(), "", 0.5, 0, 0, 25, minNPSChange);
    auto maxNPSSlider = BeatSaberUI::CreateSliderSetting(NPSSliderLayout->get_transform(), "", 0.5, 0, 0, 25, maxNPSChange);

    reinterpret_cast<UnityEngine::RectTransform*>(minNPSSlider->slider->get_transform())->set_sizeDelta({20, 1});
    reinterpret_cast<UnityEngine::RectTransform*>(maxNPSSlider->slider->get_transform())->set_sizeDelta({20, 1});

    auto scoresaberTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    auto scoresaberText = BeatSaberUI::CreateText(scoresaberTextLayout->get_transform(), "[ ScoreSaber ]");
    scoresaberText->set_fontSize(3.5);
    scoresaberText->set_alignment(TMPro::TextAlignmentOptions::Center);
    scoresaberText->set_fontStyle(TMPro::FontStyles::Underline);

    std::vector<std::string> rankedFilterOptions = {"Show all", "Only Ranked", "Hide Ranked"};
    std::function<void(std::string_view)> rankedFilterChange = [=](std::string_view value) {
        if(value == "Show All") 
            filterOptions->rankedType = FilterOptions::RankedFilterType::All;
        if(value == "Only Ranked") 
            filterOptions->rankedType = FilterOptions::RankedFilterType::OnlyRanked;
        if(value == "Hide Ranked") 
            filterOptions->rankedType = FilterOptions::RankedFilterType::HideRanked;
        Sort();
    };
    auto rankedFilterDropdown = BeatSaberUI::CreateDropdown(leftOptionsLayout->get_transform(), "Ranked Status", "Show all", rankedFilterOptions, rankedFilterChange);

    std::function<void(float)> minStarChange = [=](float value) {
        filterOptions->minStars = value;
        Sort();
    };
    std::function<void(float)> maxStarChange = [=](float value) {
        filterOptions->maxStars = value;
        Sort();
    };

    auto rankedStarLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    rankedStarLayout->set_spacing(2);
    auto rankedStarSliders = BeatSaberUI::CreateHorizontalLayoutGroup(rankedStarLayout->get_transform());
    rankedStarSliders->set_spacing(-2);
    auto rankedStarLabels = BeatSaberUI::CreateHorizontalLayoutGroup(rankedStarLayout->get_transform());

    auto rankedStarLabel = BeatSaberUI::CreateText(rankedStarLabels->get_transform(), "Stars");
    rankedStarLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

    auto minStarSlider = BeatSaberUI::CreateSliderSetting(rankedStarLayout->get_transform(), "", 0.2, 0, 0, 13, minStarChange);
    auto maxStarSlider = BeatSaberUI::CreateSliderSetting(rankedStarLayout->get_transform(), "", 0.2, 0, 0, 14, maxStarChange);

    reinterpret_cast<UnityEngine::RectTransform*>(minStarSlider->slider->get_transform())->set_sizeDelta({20, 1});
    reinterpret_cast<UnityEngine::RectTransform*>(maxStarSlider->slider->get_transform())->set_sizeDelta({20, 1});

    //RightSide
    auto rightOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(filterOptionsLayout->get_transform());
    auto rightOptionsLayoutElement = rightOptionsLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    /*auto leftOptionsLayoutBG = leftOptionsLayout->get_gameObject()->AddComponent<Backgroundable*>();
    leftOptionsLayoutBG->ApplyBackground(il2cpp_utils::newcsstr("round-rect-panel"));*/
    rightOptionsLayout->set_childForceExpandHeight(false);
    rightOptionsLayout->set_childControlHeight(false);
    rightOptionsLayout->set_childScaleWidth(true);
    rightOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,0,0,0));
    rightOptionsLayoutElement->set_preferredWidth(65);

    auto beatsaverTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(rightOptionsLayout->get_transform());
    auto beatsaverText = BeatSaberUI::CreateText(beatsaverTextLayout->get_transform(), "[ BeatSaver ]");
    beatsaverText->set_fontSize(3.5);
    beatsaverText->set_alignment(TMPro::TextAlignmentOptions::Center);
    beatsaverText->set_fontStyle(TMPro::FontStyles::Underline);

    QuestUI::SliderSetting* minUploadDateSlider;
    std::function<void(float)> minUploadDateChange = [=](float value) {
        //Divided because for some reason it likes too add 4 extra digits at the end.
        int val = BetterSongSearch::GetDateAfterMonths(1525136400, value).time_since_epoch().count()/10000;
        char date[100];
        struct tm *t = gmtime(reinterpret_cast<const time_t*>(&val));
        strftime(date, sizeof(date), "%b %G", t);
        getLogger().info("%s", std::to_string(val).c_str());
        getLogger().info("%s", date);
    };
    QuestUI::SliderSetting* minRatingSlider;
    std::function<void(float)> minRatingChange = [=](float value) {
        filterOptions->minRating = value;
        Sort();
    };
    QuestUI::SliderSetting* minVotesSlider;
    std::function<void(float)> minVotesChange = [=](float value) {
        filterOptions->minVotes = value;
        Sort();
    };

    minUploadDateSlider = BeatSaberUI::CreateSliderSetting(rightOptionsLayout->get_transform(), "Min upload date", 1, GetMonthsSinceDate(1525136400), 0, GetMonthsSinceDate(1525136400), minUploadDateChange);
    minUploadDateSlider->get_gameObject()->AddComponent<BetterSongSearch::UI::SliderFormatter*>()->formatFunction = 
    [](float value)
    {
        return std::to_string(value);
    };
    minRatingSlider = BeatSaberUI::CreateSliderSetting(rightOptionsLayout->get_transform(), "Minimum Rating", 5, 0, 0, 90, minRatingChange);
    minRatingSlider->get_gameObject()->AddComponent<BetterSongSearch::UI::SliderFormatter*>()->formatFunction = 
    [](float value)->std::string
    { 
        return "%";
    };
    minVotesSlider = BeatSaberUI::CreateSliderSetting(rightOptionsLayout->get_transform(), "Minimum Votes", 1, 0, 0, 100, minVotesChange);

    auto chardifTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(rightOptionsLayout->get_transform());
    auto chardifText = BeatSaberUI::CreateText(chardifTextLayout->get_transform(), "[ Characteristic / Difficulty ]");
    chardifText->set_fontSize(3.5);
    chardifText->set_alignment(TMPro::TextAlignmentOptions::Center);
    chardifText->set_fontStyle(TMPro::FontStyles::Underline);

    std::vector<std::string> charFilterOptions = {"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
    std::function<void(std::string_view)> charFilterChange = [=](std::string_view value) {
        if(value == "Any") filterOptions->charFilter = FilterOptions::CharFilterType::All;
        if(value == "Custom") filterOptions->charFilter = FilterOptions::CharFilterType::Custom;
        if(value == "Standard") filterOptions->charFilter = FilterOptions::CharFilterType::Standard;
        if(value == "One Saber") filterOptions->charFilter = FilterOptions::CharFilterType::OneSaber;
        if(value == "No Arrows") filterOptions->charFilter = FilterOptions::CharFilterType::NoArrows;
        if(value == "90 Degrees") filterOptions->charFilter = FilterOptions::CharFilterType::NinetyDegrees;
        if(value == "360 Degrees") filterOptions->charFilter = FilterOptions::CharFilterType::ThreeSixtyDegrees;
        if(value == "LightShow") filterOptions->charFilter = FilterOptions::CharFilterType::LightShow;
        if(value == "Lawless") filterOptions->charFilter = FilterOptions::CharFilterType::Lawless;
        Sort();
    };
    auto charDropdown = BeatSaberUI::CreateDropdown(rightOptionsLayout->get_transform(), "Characteristic", "Any", charFilterOptions, charFilterChange);
    std::vector<std::string> diffFilterOptions = {"Any", "Easy", "Normal", "Hard", "Expert", "Expert+"};
    std::function<void(std::string_view)> diffFilterChange = [=](std::string_view value) {
        if(value == "Any") filterOptions->difficultyFilter = FilterOptions::DifficultyFilterType::All;
        if(value == "Easy") filterOptions->difficultyFilter = FilterOptions::DifficultyFilterType::Easy;
        if(value == "Normal") filterOptions->difficultyFilter = FilterOptions::DifficultyFilterType::Normal;
        if(value == "Hard") filterOptions->difficultyFilter = FilterOptions::DifficultyFilterType::Hard;
        if(value == "Expert") filterOptions->difficultyFilter = FilterOptions::DifficultyFilterType::Expert;
        if(value == "Expert+") filterOptions->difficultyFilter = FilterOptions::DifficultyFilterType::ExpertPlus;
        Sort();
    };
    auto diffDropdown = BeatSaberUI::CreateDropdown(rightOptionsLayout->get_transform(), "Difficulty", "Any", diffFilterOptions, diffFilterChange);

    auto modsTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(rightOptionsLayout->get_transform());
    auto modsText = BeatSaberUI::CreateText(modsTextLayout->get_transform(), "[ Mods ]");
    modsText->set_fontSize(3.5);
    modsText->set_alignment(TMPro::TextAlignmentOptions::Center);
    modsText->set_fontStyle(TMPro::FontStyles::Underline);

    std::vector<std::string> modsOptions = {"Any", "Noodle Extensions", "Mapping Extensions", "Chroma", "Cinema"};
    std::function<void(std::string_view)> modsChange = [=](std::string_view value) {

    };
    auto modsDropdown = BeatSaberUI::CreateDropdown(rightOptionsLayout->get_transform(), "Requirement", "Any", modsOptions, modsChange);
}