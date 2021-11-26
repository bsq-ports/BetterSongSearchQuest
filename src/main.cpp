#include "main.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/SliderSetting.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/SliderFormatter.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/register.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator_State.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/BeatmapDifficultyMask.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/QuestAppInit.hpp"

using namespace QuestUI;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static auto* logger = new Logger(modInfo);
    return *logger;
}

MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &GlobalNamespace::MainMenuViewController::DidActivate, void, GlobalNamespace::MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
}

MAKE_HOOK_MATCH(BetterSongSearch_HandleSelectedSong, &GlobalNamespace::LevelSelectionNavigationController::Setup, void, GlobalNamespace::LevelSelectionNavigationController* self, GlobalNamespace::SongPackMask songPackMask, GlobalNamespace::BeatmapDifficultyMask allowedBeatmapDifficultyMask, ::Array<GlobalNamespace::BeatmapCharacteristicSO*>* notAllowedCharacteristics, bool hidePacksIfOneOrNone, bool hidePracticeButton, bool showPlayerStatsInDetailView, ::Il2CppString* actionButtonText, GlobalNamespace::IBeatmapLevelPack* levelPackToBeSelectedAfterPresent, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory startLevelCategory, GlobalNamespace::IPreviewBeatmapLevel* beatmapLevelToBeSelectedAfterPresent, bool enableCustomLevels)
{
    getLogger().info("Setup");
    if(inBSS)
    {
        getLogger().info("In BSS");
        self->levelFilteringNavigationController->SetupBeatmapLevelPacks();
        getLogger().info("Setup Packs");
        BetterSongSearch_HandleSelectedSong(self, songPackMask, allowedBeatmapDifficultyMask, notAllowedCharacteristics, hidePacksIfOneOrNone, hidePracticeButton, showPlayerStatsInDetailView, actionButtonText, levelPackToBeSelectedAfterPresent, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::All, currentLevel, enableCustomLevels);
        inBSS = false;
        return;
    }
    BetterSongSearch_HandleSelectedSong(self, songPackMask, allowedBeatmapDifficultyMask, notAllowedCharacteristics, hidePacksIfOneOrNone, hidePracticeButton, showPlayerStatsInDetailView, actionButtonText, levelPackToBeSelectedAfterPresent, startLevelCategory, beatmapLevelToBeSelectedAfterPresent, enableCustomLevels);
}

MAKE_HOOK_MATCH(BetterSongSearch_HandleSelectedSongCategory, &GlobalNamespace::SelectLevelCategoryViewController::Setup, void, GlobalNamespace::SelectLevelCategoryViewController* self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory selectedCategory, ::Array<GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>* enabledLevelCategories)
{
    if(inBSS)
    {
        BetterSongSearch_HandleSelectedSongCategory(self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::All, enabledLevelCategories);
    }
    else
    {
        BetterSongSearch_HandleSelectedSongCategory(self, selectedCategory, enabledLevelCategories);
    }
}

MAKE_HOOK_MATCH(BetterSongSearch_ScuffPackFix, &GlobalNamespace::LevelFilteringNavigationController::SetupBeatmapLevelPacks, void, GlobalNamespace::LevelFilteringNavigationController* self)
{
    if(inBSS)
    {
        self->levelPackIdToBeSelectedAfterPresent = il2cpp_utils::newcsstr("");
    }
    BetterSongSearch_ScuffPackFix(self);
}

MAKE_HOOK_MATCH(BetterSongSearch_ScuffPackFix2, &GlobalNamespace::LevelFilteringNavigationController::UpdateSecondChildControllerContent, void, GlobalNamespace::LevelFilteringNavigationController* self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory levelCategory)
{
    if(inBSS)
    {
        self->levelPackIdToBeSelectedAfterPresent = nullptr;
    }
    BetterSongSearch_ScuffPackFix2(self, levelCategory);
}

MAKE_HOOK_MATCH(BetterSongSearch_BackButtonWasPressed, &GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator::BackButtonWasPressed, void, GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator* self, HMUI::ViewController* topViewController)
{
    if(fcInstance != nullptr)
    {
        if(self->parentFlowCoordinator == fcInstance)
        {
            if(self->get_isInPracticeView())
            {
                self->DismissPracticeViewController(nullptr, true);
            }
            fcInstance->DismissFlowCoordinator(self, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, false);
        }
        else
        {
            BetterSongSearch_BackButtonWasPressed(self, topViewController);
        }
        return;
    }
    BetterSongSearch_BackButtonWasPressed(self, topViewController);
}

MAKE_HOOK(QuestUI_TextForValue, nullptr, Il2CppString*, QuestUI::SliderSetting* self, float value)
{
    getLogger().info("TextForValue");

    auto formatter = self->GetComponent<BetterSongSearch::UI::SliderFormatter*>();
    if(formatter != nullptr)
    {
        return il2cpp_utils::newcsstr(formatter->formatFunction(value));
    }
    else
    {
        return QuestUI_TextForValue(self, value);
    }
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
    std::thread([]{
        songList = SDC_wrapper::BeatStarSong::GetAllSongs();
        getLogger().info("yes");
    }).detach();
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing hooks...");
    // Install our hooks (none defined yet)
    getLogger().info("Installed all hooks!");

    QuestUI::Init();
    getLogger().info("Successfully installed Settings UI!");

    INSTALL_HOOK(getLogger(), BetterSongSearch_HandleSelectedSong);
    INSTALL_HOOK(getLogger(), BetterSongSearch_HandleSelectedSongCategory);
    INSTALL_HOOK(getLogger(), BetterSongSearch_BackButtonWasPressed);
    INSTALL_HOOK(getLogger(), BetterSongSearch_ScuffPackFix);
    INSTALL_HOOK(getLogger(), BetterSongSearch_ScuffPackFix2);
    //INSTALL_HOOK_DIRECT(getLogger(), QuestUI_TextForValue, (void*)QuestUI::SliderSetting::TextForValue);

    custom_types::Register::AutoRegister();
    modInfo.id = "Better Song Search";
    QuestUI::Register::RegisterMainMenuModSettingsFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>(modInfo);
    modInfo.id = ID;
}