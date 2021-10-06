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

MAKE_HOOK_MATCH(BetterSongSearch_HandleSoloFreePlayFlowCoordinatorDidFinish, &GlobalNamespace::MainFlowCoordinator::HandleSoloFreePlayFlowCoordinatorDidFinish, void, GlobalNamespace::MainFlowCoordinator* self, GlobalNamespace::LevelSelectionFlowCoordinator* flowCoordinator)
{
    if(fcInstance != nullptr)
    {
        if(fcInstance->isActivated)
        {
            fcInstance->DismissFlowCoordinator(flowCoordinator, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, false);
            self->PresentFlowCoordinator(fcInstance, nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false);
        }
        else
        {
            BetterSongSearch_HandleSoloFreePlayFlowCoordinatorDidFinish(self, flowCoordinator);
        }
        return;
    }
    BetterSongSearch_HandleSoloFreePlayFlowCoordinatorDidFinish(self, flowCoordinator);
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

    //INSTALL_HOOK(getLogger(), BetterSongSearch_HandleSoloFreePlayFlowCoordinatorDidFinish);
    INSTALL_HOOK(getLogger(), BetterSongSearch_BackButtonWasPressed);

    custom_types::Register::AutoRegister();

    QuestUI::Register::RegisterMainMenuModSettingsFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>(modInfo);
}