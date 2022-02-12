#include "main.hpp"

#include "questui/shared/QuestUI.hpp"

#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

#include "custom-types/shared/register.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"

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
    static auto* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
    std::thread([]{
        auto songs = SDC_wrapper::BeatStarSong::GetAllSongs();
        DataHolder::songList = std::unordered_set(songs.begin(), songs.end());
        getLogger().info("yes");
        DataHolder::loadedSDC = true;
    }).detach();
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    QuestUI::Init();

    custom_types::Register::AutoRegister();
    modInfo.id = "Better Song Search";
    QuestUI::Register::RegisterMainMenuModSettingsFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>(modInfo);
    modInfo.id = MOD_ID;
}