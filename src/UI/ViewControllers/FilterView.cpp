#include "UI/ViewControllers/FilterView.hpp"
#include "main.hpp"

#include "PluginConfig.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "questui/shared/CustomTypes/Components/Settings/SliderSetting.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
using namespace QuestUI;

#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"

#include "HMUI/CurvedCanvasSettingsHelper.hpp"
#include "HMUI/TimeSlider.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/SimpleTextDropdown.hpp"
#include "HMUI/DropdownWithTableView.hpp"
#include "HMUI/ModalView.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"

#include "DateUtils.hpp"
#include "FilterOptions.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include<sstream>

#include "questui_components/shared/concepts.hpp"

#include <fmt/chrono.h>

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

static const std::chrono::system_clock::time_point BEATSAVER_EPOCH_TIME_POINT{std::chrono::seconds(FilterOptions::BEATSAVER_EPOCH)};

UnityEngine::UI::VerticalLayoutGroup* filterViewLayout;

template<typename T = std::string, typename V = std::string_view>
requires(QUC::IsQUCConvertible<T, V>)
constexpr size_t getIndex(std::span<T const> const v, V const& k)
{
    auto it = std::find(v.begin(), v.end(), k);
    if (it != v.end())
    {
        return std::distance(v.begin(), it);
    }
    else {
        return -1;
    }
}
template<typename T = std::string>
constexpr size_t getIndex(std::span<T const> const v, T const& k)
{
    return getIndex<T, T>(v, k);
}


constexpr size_t getIndex(std::span<StringW const> const v, StringW const k)
{
    return getIndex<StringW, StringW>(v, k);
}

UnityEngine::Sprite* GetBGSprite(std::string str)
{
    return QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>(),
                                     [str](UnityEngine::Sprite* x) {
                                         return to_utf8(csstrtostr(x->get_name())) == str;
                                     });
}


void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;
    auto filterBorderSprite = GetBGSprite("RoundRect10BorderFade");

    std::vector<StringW> downloadFilterOptions({"Show All", "Only Downloaded", "Hide Downloaded"});
    std::vector<StringW> scoreFilterOptions = {"Show All", "Hide Passed", "Only Passed"};
    std::vector<StringW> rankedFilterOptions = {"Show All", "Hide Ranked", "Only Ranked"};
    std::vector<StringW> charFilterOptions = {"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
    std::vector<StringW> diffFilterOptions = {"Any", "Easy", "Normal", "Hard", "Expert", "Expert+"};
    std::vector<StringW> modsOptions = {"Any", "Noodle Extensions", "Mapping Extensions", "Chroma", "Cinema"};

    auto& filterOptions = DataHolder::filterOptions;

    get_rectTransform()->set_offsetMax(UnityEngine::Vector2(20, 22));
    get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(130);

    // This could've been great in QUC v2
    // but
    // I cannot be bothered to convert this further
    // and it just works:tm:
    filterViewLayout = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    auto filterViewLayoutElement = filterViewLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    filterViewLayoutElement->set_preferredWidth(130);
    filterViewLayout->set_childControlHeight(false);
    filterViewLayout->get_gameObject()->AddComponent<HMUI::Touchable*>();
    filterViewLayout->get_rectTransform()->set_anchorMin(UnityEngine::Vector2(filterViewLayout->get_rectTransform()->get_anchorMin().x, 1));

    //Top Bar
    auto topBar = BeatSaberUI::CreateHorizontalLayoutGroup(filterViewLayout->get_transform());
    topBar->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
    topBar->set_childControlWidth(false);

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
    imageView->set_material(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>().First(
            [](UnityEngine::Material* x) {
                return x->get_name() == "AnimatedButton";
            }));
    imageView->SetAllDirty();
    imageView->curvedCanvasSettingsHelper->Reset();

    std::function<void(bool)> titleToggleChange = [](bool value) {
        getPluginConfig().ReturnToBSS.SetValue(value);
    };
    auto topBarTitleLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    auto topBarTitleLayoutElement = topBarTitleLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    topBarTitleLayoutElement->set_ignoreLayout(true);
    topBarTitleLayout->set_padding(UnityEngine::RectOffset::New_ctor(10, 0, 0, 0));

    auto topBarTitle = BeatSaberUI::CreateText(topBar->get_transform(), "FILTERS", true);
    topBarTitle->set_fontSize(7);
    topBarTitle->set_alignment(TMPro::TextAlignmentOptions::Center);

    auto topBarToggle = BeatSaberUI::CreateToggle(topBarTitle->get_transform(), "", getPluginConfig().ReturnToBSS.GetValue(), titleToggleChange);
    BeatSaberUI::AddHoverHint(topBarToggle, "Toggle returning to this menu from Solo.");

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
    leftOptionsLayout->set_spacing(2.0f);
    leftOptionsLayoutElement->set_preferredWidth(65);

    {
        auto generalOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(leftOptionsLayout->get_transform());
        //generalOptionsLayout->set_childControlHeight(false);
        generalOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,1,1));

        auto layoutE = generalOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutE->set_preferredWidth(64);

        auto bg = generalOptionsLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackground(il2cpp_utils::newcsstr("panel-top"));
        auto bgImg = reinterpret_cast<HMUI::ImageView*>(bg->background);
        bgImg->dyn__skew() = 0.0f;
        bgImg->set_overrideSprite(nullptr);
        bgImg->set_sprite(filterBorderSprite);
        bgImg->set_color(UnityEngine::Color(0, 0.7f, 1.0f, 0.4f));

        auto generalTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(generalOptionsLayout->get_transform());
        auto generalText = BeatSaberUI::CreateText(generalTextLayout->get_transform(), "<color=#DDD>[ General ]");
        generalText->set_fontSize(3.5);
        generalText->set_alignment(TMPro::TextAlignmentOptions::Center);
        generalText->set_fontStyle(TMPro::FontStyles::Underline);
        generalText->set_color(UnityEngine::Color(102.0f,153.0f,187.0f, 1.0f));

        std::function<void(StringW)> downloadFilterChange = [downloadFilterOptions](StringW value) {
            filterOptions.downloadType = (FilterOptions::DownloadFilterType) getIndex(downloadFilterOptions, value);
            getPluginConfig().DownloadType.SetValue((int) getIndex(downloadFilterOptions, value));
            std::thread([]{
                Sort(true);
            }).detach();
        };
        auto downloadFilterDropdown = BeatSaberUI::CreateDropdown(generalOptionsLayout->get_transform(), "Downloaded", downloadFilterOptions[(int)getPluginConfig().DownloadType.GetValue()], downloadFilterOptions, downloadFilterChange);

        std::function<void(StringW)> scoreFilterChange = [scoreFilterOptions](StringW value) {
            filterOptions.localScoreType = (FilterOptions::LocalScoreFilterType) getIndex(scoreFilterOptions, value);
            getPluginConfig().LocalScoreType.SetValue((int) getIndex(scoreFilterOptions, value));
            std::thread([]{
                Sort(true);
            }).detach();
        };
        auto scoreFilterDropdown = BeatSaberUI::CreateDropdown(generalOptionsLayout->get_transform(), "Local score", scoreFilterOptions[(int)getPluginConfig().LocalScoreType.GetValue()], scoreFilterOptions, scoreFilterChange);

        std::function<void(float)> minLengthChange = [](float value) {
            filterOptions.minLength = value * 60;
            getPluginConfig().MinLength.SetValue(value * 60);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<void(float)> maxLengthChange = [](float value) {
            filterOptions.maxLength = value * 60;
            getPluginConfig().MaxLength.SetValue(value * 60);
            std::thread([]{
                Sort(true);
            }).detach();
        };

        auto lengthSliderLayout = BeatSaberUI::CreateHorizontalLayoutGroup(generalOptionsLayout->get_transform());
        lengthSliderLayout->set_spacing(2);
        auto lengthSliders = BeatSaberUI::CreateHorizontalLayoutGroup(lengthSliderLayout->get_transform());
        lengthSliders->set_spacing(-2);
        auto lengthLabels = BeatSaberUI::CreateHorizontalLayoutGroup(lengthSliderLayout->get_transform());

        auto lengthLabel = BeatSaberUI::CreateText(lengthLabels->get_transform(), "Length");
        lengthLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

        auto minLengthSlider = BeatSaberUI::CreateSliderSetting(lengthSliderLayout->get_transform(), "", 0.25, getPluginConfig().MinLength.GetValue() / 60, 0, 15, minLengthChange);
        auto maxLengthSlider = BeatSaberUI::CreateSliderSetting(lengthSliderLayout->get_transform(), "", 0.25, getPluginConfig().MaxLength.GetValue() / 60, 0, 15, maxLengthChange);

        std::function<std::string(float)> minLengthSliderFormatFunction = [](float value) {
            float totalSeconds = value * 60;
            int minutes = ((int)totalSeconds % 3600) / 60;
            int seconds = (int)totalSeconds % 60;

            return fmt::format("{:02}:{:02}", minutes, seconds);
        };
        std::function<std::string(float)> maxLengthSliderFormatFunction = [](float value) {
            float totalSeconds = value * 60;
            int minutes = ((int)totalSeconds % 3600) / 60;
            int seconds = (int)totalSeconds % 60;

            return fmt::format("{:02}:{:02}", minutes, seconds);
        };

        minLengthSlider->FormatString = minLengthSliderFormatFunction;
        maxLengthSlider->FormatString = maxLengthSliderFormatFunction;

        reinterpret_cast<UnityEngine::RectTransform*>(minLengthSlider->slider->get_transform())->set_sizeDelta({20, 1});
        reinterpret_cast<UnityEngine::RectTransform*>(maxLengthSlider->slider->get_transform())->set_sizeDelta({20, 1});
    }
    {
        auto mappingOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(leftOptionsLayout->get_transform());
        //mappingOptionsLayout->set_childControlHeight(false);
        mappingOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,1,1));

        auto layoutE = mappingOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutE->set_preferredWidth(64);

        auto bg = mappingOptionsLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackground(il2cpp_utils::newcsstr("panel-top"));
        auto bgImg = reinterpret_cast<HMUI::ImageView*>(bg->background);
        bgImg->dyn__skew() = 0.0f;
        bgImg->set_overrideSprite(nullptr);
        bgImg->set_sprite(filterBorderSprite);
        bgImg->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));

        auto mappingTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(mappingOptionsLayout->get_transform());
        auto mappingText = BeatSaberUI::CreateText(mappingTextLayout->get_transform(), "<color=#DDD>[ Mapping ]");
        mappingText->set_fontSize(3.5);
        mappingText->set_alignment(TMPro::TextAlignmentOptions::Center);
        mappingText->set_fontStyle(TMPro::FontStyles::Underline);
        mappingText->set_color(UnityEngine::Color(102.0f,153.0f,187.0f, 1.0f));

        std::function<void(float)> minNJSChange = [](float value) {
            filterOptions.minNJS = value;
            getPluginConfig().MinNJS.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<void(float)> maxNJSChange = [](float value) {
            filterOptions.maxNJS = value;
            getPluginConfig().MaxNJS.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<std::string(float)> minNJSFormat = [](float value) {
            return fmt::format("{:.1f}", value);
        };
        std::function<std::string(float)> maxNJSFormat = [](float value) {
            return fmt::format("{:.1f}", value);
        };


        auto NJSSliderLayout = BeatSaberUI::CreateHorizontalLayoutGroup(mappingOptionsLayout->get_transform());
        NJSSliderLayout->set_spacing(2);
        auto NJSSliders = BeatSaberUI::CreateHorizontalLayoutGroup(NJSSliderLayout->get_transform());
        NJSSliders->set_spacing(-2);
        auto NJSLabels = BeatSaberUI::CreateHorizontalLayoutGroup(NJSSliderLayout->get_transform());

        auto NJSLabel = BeatSaberUI::CreateText(NJSLabels->get_transform(), "NJS");
        NJSLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

        auto minNJSSlider = BeatSaberUI::CreateSliderSetting(NJSSliderLayout->get_transform(), "", 0.5, getPluginConfig().MinNJS.GetValue(), 0, 25, minNJSChange);
        auto maxNJSSlider = BeatSaberUI::CreateSliderSetting(NJSSliderLayout->get_transform(), "", 0.5, getPluginConfig().MaxNJS.GetValue(), 0, 25, maxNJSChange);
        minNJSSlider->FormatString = minNJSFormat;
        maxNJSSlider->FormatString = maxNJSFormat;

        reinterpret_cast<UnityEngine::RectTransform*>(minNJSSlider->slider->get_transform())->set_sizeDelta({20, 1});
        reinterpret_cast<UnityEngine::RectTransform*>(maxNJSSlider->slider->get_transform())->set_sizeDelta({20, 1});

        std::function<void(float)> minNPSChange = [](float value) {
            filterOptions.minNPS = value;
            getPluginConfig().MinNPS.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<void(float)> maxNPSChange = [](float value) {
            filterOptions.maxNPS = value;
            getPluginConfig().MaxNPS.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<std::string(float)> minNPSFormat = [](float value) {
            return fmt::format("{:.1f}", value);
        };
        std::function<std::string(float)> maxNPSFormat = [](float value) {
            return fmt::format("{:.1f}", value);
        };

        auto NPSSliderLayout = BeatSaberUI::CreateHorizontalLayoutGroup(mappingOptionsLayout->get_transform());
        NPSSliderLayout->set_spacing(2);
        auto NPSSliders = BeatSaberUI::CreateHorizontalLayoutGroup(NPSSliderLayout->get_transform());
        NPSSliders->set_spacing(-2);
        auto NPSLabels = BeatSaberUI::CreateHorizontalLayoutGroup(NPSSliderLayout->get_transform());

        auto NPSLabel = BeatSaberUI::CreateText(NPSLabels->get_transform(), "Notes/s");
        NPSLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

        auto minNPSSlider = BeatSaberUI::CreateSliderSetting(NPSSliderLayout->get_transform(), "", 0.5, getPluginConfig().MinNPS.GetValue(), 0, 12, minNPSChange);
        auto maxNPSSlider = BeatSaberUI::CreateSliderSetting(NPSSliderLayout->get_transform(), "", 0.5, getPluginConfig().MaxNPS.GetValue(), 0, 12, maxNPSChange);

        minNPSSlider->FormatString = minNPSFormat;
        maxNPSSlider->FormatString = maxNPSFormat;

        reinterpret_cast<UnityEngine::RectTransform*>(minNPSSlider->slider->get_transform())->set_sizeDelta({20, 1});
        reinterpret_cast<UnityEngine::RectTransform*>(maxNPSSlider->slider->get_transform())->set_sizeDelta({20, 1});
    }
    {
        auto scoreSaberOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(leftOptionsLayout->get_transform());
        //scoreSaberOptionsLayout->set_childControlHeight(false);
        scoreSaberOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,1,1));

        auto layoutE = scoreSaberOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutE->set_preferredWidth(64);

        auto bg = scoreSaberOptionsLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackground(il2cpp_utils::newcsstr("panel-top"));
        auto bgImg = reinterpret_cast<HMUI::ImageView*>(bg->background);
        bgImg->dyn__skew() = 0.0f;
        bgImg->set_overrideSprite(nullptr);
        bgImg->set_sprite(filterBorderSprite);
        bgImg->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));

        auto scoresaberTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(scoreSaberOptionsLayout->get_transform());
        auto scoresaberText = BeatSaberUI::CreateText(scoresaberTextLayout->get_transform(), "<color=#DDD>[ ScoreSaber ]");
        scoresaberText->set_fontSize(3.5);
        scoresaberText->set_alignment(TMPro::TextAlignmentOptions::Center);
        scoresaberText->set_fontStyle(TMPro::FontStyles::Underline);
        scoresaberText->set_color(UnityEngine::Color(102.0f,153.0f,187.0f, 1.0f));


        std::function<void(StringW)> rankedFilterChange = [rankedFilterOptions](StringW value) {
            if(value == "Show All")
                filterOptions.rankedType = FilterOptions::RankedFilterType::All;
            if(value == "Only Ranked")
                filterOptions.rankedType = FilterOptions::RankedFilterType::OnlyRanked;
            if(value == "Hide Ranked")
                filterOptions.rankedType = FilterOptions::RankedFilterType::HideRanked;
            getPluginConfig().RankedType.SetValue(getIndex(rankedFilterOptions, value));
            std::thread([]{
                Sort(true);
            }).detach();
        };
        auto rankedFilterDropdown = BeatSaberUI::CreateDropdown(scoreSaberOptionsLayout->get_transform(), "Ranked Status", rankedFilterOptions[(int)getPluginConfig().RankedType.GetValue()], rankedFilterOptions, rankedFilterChange);

        std::function<void(float)> minStarChange = [](float value) {
            filterOptions.minStars = value;
            getPluginConfig().MinStars.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<void(float)> maxStarChange = [](float value) {
            filterOptions.maxStars = value;
            getPluginConfig().MaxStars.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        std::function<std::string(float)> minStarFormat = [](float value) {
            return fmt::format("{:.1f}", value);
        };
        std::function<std::string(float)> maxStarFormat = [](float value) {
            return fmt::format("{:.1f}", value);
        };


        auto rankedStarLayout = BeatSaberUI::CreateHorizontalLayoutGroup(scoreSaberOptionsLayout->get_transform());
        rankedStarLayout->set_spacing(2);
        auto rankedStarSliders = BeatSaberUI::CreateHorizontalLayoutGroup(rankedStarLayout->get_transform());
        rankedStarSliders->set_spacing(-2);
        auto rankedStarLabels = BeatSaberUI::CreateHorizontalLayoutGroup(rankedStarLayout->get_transform());

        auto rankedStarLabel = BeatSaberUI::CreateText(rankedStarLabels->get_transform(), "Stars");
        rankedStarLabel->set_alignment(TMPro::TextAlignmentOptions::Center);

        auto minStarSlider = BeatSaberUI::CreateSliderSetting(rankedStarLayout->get_transform(), "", 0.2, getPluginConfig().MinStars.GetValue(), 0, 13, minStarChange);
        auto maxStarSlider = BeatSaberUI::CreateSliderSetting(rankedStarLayout->get_transform(), "", 0.2, getPluginConfig().MaxStars.GetValue(), 0, 14, maxStarChange);
        minStarSlider->FormatString = minStarFormat;
        maxStarSlider->FormatString = maxStarFormat;

        reinterpret_cast<UnityEngine::RectTransform*>(minStarSlider->slider->get_transform())->set_sizeDelta({20, 1});
        reinterpret_cast<UnityEngine::RectTransform*>(maxStarSlider->slider->get_transform())->set_sizeDelta({20, 1});
    }

    //RightSide
    auto rightOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(filterOptionsLayout->get_transform());
    auto rightOptionsLayoutElement = rightOptionsLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    rightOptionsLayout->set_childForceExpandHeight(false);
    rightOptionsLayout->set_spacing(2.0f);
    rightOptionsLayoutElement->set_preferredWidth(65);

    {
        auto beatSaverOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(rightOptionsLayout->get_transform());
        //beatSaverOptionsLayout->set_childControlHeight(false);
        beatSaverOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,1,1));

        auto layoutE = beatSaverOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutE->set_preferredWidth(64);

        auto bg = beatSaverOptionsLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackground(il2cpp_utils::newcsstr("panel-top"));
        auto bgImg = reinterpret_cast<HMUI::ImageView*>(bg->background);
        bgImg->dyn__skew() = 0.0f;
        bgImg->set_overrideSprite(nullptr);
        bgImg->set_sprite(filterBorderSprite);
        bgImg->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));

        auto beatsaverTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(beatSaverOptionsLayout->get_transform());
        auto beatsaverText = BeatSaberUI::CreateText(beatsaverTextLayout->get_transform(), "<color=#DDD>[ BeatSaver ]");
        beatsaverText->set_fontSize(3.5);
        beatsaverText->set_alignment(TMPro::TextAlignmentOptions::Center);
        beatsaverText->set_fontStyle(TMPro::FontStyles::Underline);
        beatsaverText->set_color(UnityEngine::Color(102.0f,153.0f,187.0f, 1.0f));

        QuestUI::SliderSetting* minUploadDateSlider;
        std::function<void(float)> minUploadDateChange = [](float value) {
            std::chrono::time_point val = BEATSAVER_EPOCH_TIME_POINT + std::chrono::months((int)value);
            int sec = duration_cast<std::chrono::seconds>(val.time_since_epoch()).count();
            filterOptions.minUploadDate = sec;
            getPluginConfig().MinUploadDate.SetValue(sec);
            getPluginConfig().MinUploadDateInMonths.SetValue((int)value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        QuestUI::SliderSetting* minRatingSlider;
        std::function<void(float)> minRatingChange = [](float value) {
            filterOptions.minRating = value;
            getPluginConfig().MinRating.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };
        QuestUI::SliderSetting* minVotesSlider;
        std::function<void(float)> minVotesChange = [](float value) {
            filterOptions.minVotes = value;
            getPluginConfig().MinVotes.SetValue(value);
            std::thread([]{
                Sort(true);
            }).detach();
        };

        // TODO: Minimum upload date filter
        std::function<std::string(float)> minUploadDateSliderFormatFunciton = [](float monthsSinceFirstUpload) {
            auto val = BEATSAVER_EPOCH_TIME_POINT + std::chrono::months(int(monthsSinceFirstUpload));
            return fmt::format("{:%b:%Y}", fmt::localtime(val));
        };

        auto minUploadDate = 0;
        auto maxUploadDate = GetMonthsSinceDate(FilterOptions::BEATSAVER_EPOCH);

        minUploadDateSlider = BeatSaberUI::CreateSliderSetting(beatSaverOptionsLayout->get_transform(), "Min upload date", 1, getPluginConfig().MinUploadDateInMonths.GetValue(), minUploadDate, maxUploadDate, minUploadDateChange);
        minUploadDateSlider->FormatString = minUploadDateSliderFormatFunciton;

        std::function<std::string(float)> minRatingSliderFormatFunction = [](float value) {
            return fmt::format("{:.1f}%", value);
        };

        minRatingSlider = BeatSaberUI::CreateSliderSetting(beatSaverOptionsLayout->get_transform(), "Minimum Rating", 5, getPluginConfig().MinRating.GetValue(), 0, 90, minRatingChange);
        minRatingSlider->FormatString = minRatingSliderFormatFunction;
        minVotesSlider = BeatSaberUI::CreateSliderSetting(beatSaverOptionsLayout->get_transform(), "Minimum Votes", 1, getPluginConfig().MinVotes.GetValue(), 0, 100, minVotesChange);

        /*std::function<void(std::string_view)> UploadersChange = [=](std::string_view value) {
            filterOptions->uploaderString = value;
            std::vector<std::string> result;
            std::stringstream s_stream(filterOptions->uploaderString); //create string stream from the string
            while(s_stream.good()) {
                std::string substr;
                getline(s_stream, substr, ','); //get first string delimited by comma
                result.push_back(substr);
            }
            filterOptions->uploaders = result;
            std::thread([]{
                Sort(true);
            }).detach();
        };*/

        //auto uploaderInput = BeatSaberUI::CreateStringSetting(beatSaverOptionsLayout->get_transform(), "Uploader(s)", "", UploadersChange);
    }

    {
        auto charDiffOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(rightOptionsLayout->get_transform());
        //charDiffOptionsLayout->set_childControlHeight(false);
        charDiffOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,1,1));

        auto layoutE = charDiffOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutE->set_preferredWidth(64);

        auto bg = charDiffOptionsLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackground(il2cpp_utils::newcsstr("panel-top"));
        auto bgImg = reinterpret_cast<HMUI::ImageView*>(bg->background);
        bgImg->dyn__skew() = 0.0f;
        bgImg->set_overrideSprite(nullptr);
        bgImg->set_sprite(filterBorderSprite);
        bgImg->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));

        auto charDiffTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(charDiffOptionsLayout->get_transform());
        auto charDiffText = BeatSaberUI::CreateText(charDiffTextLayout->get_transform(), "<color=#DDD>[ Characteristic / Difficulty ]");
        charDiffText->set_fontSize(3.5);
        charDiffText->set_alignment(TMPro::TextAlignmentOptions::Center);
        charDiffText->set_fontStyle(TMPro::FontStyles::Underline);
        charDiffText->set_color(UnityEngine::Color(102.0f,153.0f,187.0f, 1.0f));


        std::function<void(StringW)> charFilterChange = [charFilterOptions](StringW value) {
            if(value == "Any") filterOptions.charFilter = FilterOptions::CharFilterType::All;
            else if(value == "Custom") filterOptions.charFilter = FilterOptions::CharFilterType::Custom;
            else if(value == "Standard") filterOptions.charFilter = FilterOptions::CharFilterType::Standard;
            else if(value == "One Saber") filterOptions.charFilter = FilterOptions::CharFilterType::OneSaber;
            else if(value == "No Arrows") filterOptions.charFilter = FilterOptions::CharFilterType::NoArrows;
            else if(value == "90 Degrees") filterOptions.charFilter = FilterOptions::CharFilterType::NinetyDegrees;
            else if(value == "360 Degrees") filterOptions.charFilter = FilterOptions::CharFilterType::ThreeSixtyDegrees;
            else if(value == "LightShow") filterOptions.charFilter = FilterOptions::CharFilterType::LightShow;
            else if(value == "Lawless") filterOptions.charFilter = FilterOptions::CharFilterType::Lawless;
            getPluginConfig().CharacteristicType.SetValue(getIndex(charFilterOptions, value));
            std::thread([]{
                Sort(true);
            }).detach();
        };
        auto charDropdown = BeatSaberUI::CreateDropdown(charDiffOptionsLayout->get_transform(), "Characteristic", charFilterOptions[(int)getPluginConfig().CharacteristicType.GetValue()], charFilterOptions, charFilterChange);

        std::function<void(StringW)> diffFilterChange = [diffFilterOptions](StringW value) {
            if(value == "Any") filterOptions.difficultyFilter = FilterOptions::DifficultyFilterType::All;
            else if(value == "Easy") filterOptions.difficultyFilter = FilterOptions::DifficultyFilterType::Easy;
            else if(value == "Normal") filterOptions.difficultyFilter = FilterOptions::DifficultyFilterType::Normal;
            else if(value == "Hard") filterOptions.difficultyFilter = FilterOptions::DifficultyFilterType::Hard;
            else if(value == "Expert") filterOptions.difficultyFilter = FilterOptions::DifficultyFilterType::Expert;
            else if(value == "Expert+") filterOptions.difficultyFilter = FilterOptions::DifficultyFilterType::ExpertPlus;
            getPluginConfig().DifficultyType.SetValue(getIndex(diffFilterOptions, value));
            std::thread([]{
                Sort(true);
            }).detach();
        };
        auto diffDropdown = BeatSaberUI::CreateDropdown(charDiffOptionsLayout->get_transform(), "Difficulty", diffFilterOptions[(int)getPluginConfig().DifficultyType.GetValue()], diffFilterOptions, diffFilterChange);
    }
    {
        auto modsOptionsLayout = BeatSaberUI::CreateVerticalLayoutGroup(rightOptionsLayout->get_transform());
        //charDiffOptionsLayout->set_childControlHeight(false);
        modsOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,1,1));

        auto layoutE = modsOptionsLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layoutE->set_preferredWidth(64);

        auto bg = modsOptionsLayout->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackground(il2cpp_utils::newcsstr("panel-top"));
        auto bgImg = reinterpret_cast<HMUI::ImageView*>(bg->background);
        bgImg->dyn__skew() = 0.0f;
        bgImg->set_overrideSprite(nullptr);
        bgImg->set_sprite(filterBorderSprite);
        bgImg->set_color(UnityEngine::Color(0.0f, 0.7f, 1.0f, 0.4f));

        auto modsTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(modsOptionsLayout->get_transform());
        auto modsText = BeatSaberUI::CreateText(modsTextLayout->get_transform(), "<color=#DDD>[ Mods ]");
        modsText->set_fontSize(3.5);
        modsText->set_alignment(TMPro::TextAlignmentOptions::Center);
        modsText->set_fontStyle(TMPro::FontStyles::Underline);
        modsText->set_color(UnityEngine::Color(102.0f,153.0f,187.0f, 1.0f));

        std::function<void(StringW)> modsChange = [modsOptions](StringW value) {
            filterOptions.modRequirement = (FilterOptions::RequirementType) getIndex(modsOptions, value);
            getPluginConfig().RequirementType.SetValue((int) getIndex(modsOptions, value));
            std::thread([]{
                Sort(true);
            }).detach();
        };

        auto modsDropdown = BeatSaberUI::CreateDropdown(modsOptionsLayout->get_transform(), "Requirement", modsOptions[(int)getPluginConfig().RequirementType.GetValue()], modsOptions, modsChange);
#define CODEGEN_FIELD_ACCESSIBILITY
        reinterpret_cast<UnityEngine::RectTransform*>(reinterpret_cast<HMUI::DropdownWithTableView*>(modsDropdown)->modalView->get_transform())->set_pivot(Sombrero::FastVector2(0.5f, 0.3f));
    }
}