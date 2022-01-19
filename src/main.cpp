#include "main.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/SliderSetting.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

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

    QuestUI::Init();

    custom_types::Register::AutoRegister();
    modInfo.id = "Better Song Search";
    QuestUI::Register::RegisterMainMenuModSettingsFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>(modInfo);
    modInfo.id = ID;
}