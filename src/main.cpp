#include "main.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/register.hpp"
#include "custom-types/shared/coroutine.hpp"
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
#include "System/Nullable_1.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"

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

custom_types::Helpers::Coroutine coroutine(GlobalNamespace::LevelSelectionNavigationController* thing, std::function<void()> callback) {
    thing->levelFilteringNavigationController->UpdateCustomSongs();
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(UnityEngine::WaitForEndOfFrame::New_ctor()));
    callback();
    co_return;
}

MAKE_HOOK_MATCH(BetterSongSearch_HandleSelectedSong, &GlobalNamespace::LevelSelectionNavigationController::Setup, void, GlobalNamespace::LevelSelectionNavigationController* self, GlobalNamespace::SongPackMask songPackMask, GlobalNamespace::BeatmapDifficultyMask allowedBeatmapDifficultyMask, ::Array<GlobalNamespace::BeatmapCharacteristicSO*>* notAllowedCharacteristics, bool hidePacksIfOneOrNone, bool hidePracticeButton, bool showPlayerStatsInDetailView, ::Il2CppString* actionButtonText, GlobalNamespace::IBeatmapLevelPack* levelPackToBeSelectedAfterPresent, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory startLevelCategory, GlobalNamespace::IPreviewBeatmapLevel* beatmapLevelToBeSelectedAfterPresent, bool enableCustomLevels)
{
    getLogger().info("Setup");
    if(inBSS)
    {
        getLogger().info("In BSS");
        self->levelFilteringNavigationController->SetupBeatmapLevelPacks();
        BetterSongSearch_HandleSelectedSong(self, songPackMask, allowedBeatmapDifficultyMask, notAllowedCharacteristics, hidePacksIfOneOrNone, hidePracticeButton, showPlayerStatsInDetailView, actionButtonText, self->levelFilteringNavigationController->customLevelPacks->values[0], GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::Favorites, currentLevel, enableCustomLevels);
        inBSS = false;
        return;
    }
    BetterSongSearch_HandleSelectedSong(self, songPackMask, allowedBeatmapDifficultyMask, notAllowedCharacteristics, hidePacksIfOneOrNone, hidePracticeButton, showPlayerStatsInDetailView, actionButtonText, levelPackToBeSelectedAfterPresent, startLevelCategory, beatmapLevelToBeSelectedAfterPresent, enableCustomLevels);
}

MAKE_HOOK_MATCH(BetterSongSearch_HandleSelectedSongCategory, &GlobalNamespace::SelectLevelCategoryViewController::Setup, void, GlobalNamespace::SelectLevelCategoryViewController* self, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory selectedCategory, ::Array<GlobalNamespace::SelectLevelCategoryViewController::LevelCategory>* enabledLevelCategories)
{
    if(inBSS)
    {
        BetterSongSearch_HandleSelectedSongCategory(self, 4, enabledLevelCategories);
    }
    else
    {
        BetterSongSearch_HandleSelectedSongCategory(self, selectedCategory, enabledLevelCategories);
    }
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

    custom_types::Register::AutoRegister();
    modInfo.id = "Better Song Search";
    QuestUI::Register::RegisterMainMenuModSettingsFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>(modInfo);
    modInfo.id = ID;
}