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

#include "HMUI/CurvedCanvasSettingsHelper.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

UnityEngine::UI::VerticalLayoutGroup* filterViewLayout;


void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    filterViewLayout = BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
    auto filterViewLayoutElement = filterViewLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    filterViewLayoutElement->set_preferredWidth(130);
    filterViewLayout->set_childControlHeight(false);
    filterViewLayout->get_rectTransform()->set_anchorMin(UnityEngine::Vector2(filterViewLayout->get_rectTransform()->get_anchorMin().x, 1));

    //Top Bar
    auto topBar = BeatSaberUI::CreateHorizontalLayoutGroup(filterViewLayout->get_transform());
    auto topBarBG = topBar->get_gameObject()->AddComponent<Backgroundable*>();
    topBarBG->ApplyBackground(il2cpp_utils::newcsstr("panel-top-gradient"));
    auto imageView = topBarBG->GetComponentInChildren<HMUI::ImageView*>();
    imageView->set_color(UnityEngine::Color(0.05,0.71,0.94, 1));
    imageView->curvedCanvasSettingsHelper->Reset();
    std::function<void()> settingsButtonClick = [=]() {

    };
    std::function<void()> clearButtonClick = [=]() {

    };
    std::function<void()> presetsButtonClick = [=]() {

    };
    auto topBarSettingsButtonLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    auto topBarSettingsButton = BeatSaberUI::CreateUIButton(topBarSettingsButtonLayout->get_transform(), "Settings", settingsButtonClick);
    auto topBarTitleLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    auto topBarTitleLayoutElement = topBarTitleLayout->GetComponent<UnityEngine::UI::LayoutElement*>();
    topBarTitleLayoutElement->set_ignoreLayout(true);
    auto topBarTitle = BeatSaberUI::CreateText(topBarTitleLayout->get_transform(), "FILTERS", true);
    topBarTitle->set_fontSize(7);
    topBarTitle->set_alignment(TMPro::TextAlignmentOptions::Center);
    auto topBarButtonsLayout = BeatSaberUI::CreateHorizontalLayoutGroup(topBar->get_transform());
    auto topBarButtonsLayoutFitter = topBarButtonsLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    topBarButtonsLayoutFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    topBarButtonsLayout->set_spacing(2);
    topBarButtonsLayout->set_padding(UnityEngine::RectOffset::New_ctor(77,0,0,0));
    auto topBarClearButton = BeatSaberUI::CreateUIButton(topBarButtonsLayout->get_transform(), "Clear", clearButtonClick);
    auto topBarPresetsButton = BeatSaberUI::CreateUIButton(topBarButtonsLayout->get_transform(), "Presets", presetsButtonClick);

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
    auto leftOptionsLayoutBG = leftOptionsLayout->get_gameObject()->AddComponent<Backgroundable*>();
    leftOptionsLayoutBG->ApplyBackground(il2cpp_utils::newcsstr("round-rect-panel"));
    leftOptionsLayout->set_childForceExpandHeight(false);
    leftOptionsLayout->set_childControlHeight(false);
    leftOptionsLayout->set_childScaleWidth(true);
    leftOptionsLayout->set_padding(UnityEngine::RectOffset::New_ctor(0,2,0,0));
    leftOptionsLayoutElement->set_preferredWidth(65);

    auto generalTextLayout = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    auto generalText = BeatSaberUI::CreateText(generalTextLayout->get_transform(), "[ General ]");
    generalText->set_fontSize(3.5);
    generalText->set_alignment(TMPro::TextAlignmentOptions::Center);
    generalText->set_fontStyle(TMPro::FontStyles::Underline);

    std::vector<std::string> downloadFilterOptions = {"Show all", "Only downloaded", "Hide downloaded"};
    std::function<void(std::string_view)> downloadFilterChange = [=](std::string_view value) {

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
    auto horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(leftOptionsLayout->get_transform());
    auto horizontalBG = horizontal->get_gameObject()->AddComponent<Backgroundable*>();
    horizontalBG->ApplyBackground(il2cpp_utils::newcsstr("round-rect-panel"));
    horizontal->set_childControlWidth(true);
    auto horizontalElement = horizontal->GetComponent<UnityEngine::UI::LayoutElement*>();
    horizontalElement->set_preferredWidth(40);

    auto minLengthSlider = BeatSaberUI::CreateSliderSetting(horizontal->get_transform(), "Length", 0.25, 0, 0, 10, minLengthChange);
    auto maxLengthSlider = BeatSaberUI::CreateSliderSetting(horizontal->get_transform(), "", 0.25, 0, 0, 10, maxLengthChange);

    auto ourContainer = maxLengthSlider->get_transform()->get_parent();
    auto prevContainer = minLengthSlider->get_transform()->get_parent();

    ourContainer
}