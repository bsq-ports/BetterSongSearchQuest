#include "main.hpp"

#include "bsml/shared/BSML-Lite.hpp"
#include "System/Action.hpp"
#include "custom-types/shared/register.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MultiplayerResultsViewController.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "PluginConfig.hpp"
#include "UI/Manager.hpp"
#include "HMUI/TextSegmentedControlCell.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "Util/TextUtil.hpp"

#include "GlobalNamespace/CoroutineStarter.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include <regex>

#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "bsml/shared/Helpers/getters.hpp"

#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

using namespace BetterSongSearch::Util;

inline modloader::ModInfo modInfo = {MOD_ID, VERSION, GIT_COMMIT}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLoggerOld() {
    static auto* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

// Returns a logger, useful for printing debug messages
Paper::ConstLoggerContext<17UL> getLogger() {
    static auto fastContext = Paper::Logger::WithContext<MOD_ID>();
    return fastContext;
}


// Called at the early stages of game loading
extern "C" void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);

    getConfig().Load(); // Load the config file
    getPluginConfig().Init(modInfo);
    getConfig().Reload();
    getConfig().Write();
    getLoggerOld().info("Completed setup!");

    std::thread([]{
        auto& filterOptions = DataHolder::filterOptions;
        getLoggerOld().info("setting config values");
        filterOptions.downloadType = (FilterOptions::DownloadFilterType) getPluginConfig().DownloadType.GetValue();
        filterOptions.localScoreType = (FilterOptions::LocalScoreFilterType) getPluginConfig().LocalScoreType.GetValue();
        filterOptions.minLength = getPluginConfig().MinLength.GetValue();
        filterOptions.maxLength = getPluginConfig().MaxLength.GetValue();
        filterOptions.minNJS = getPluginConfig().MinNJS.GetValue();
        filterOptions.maxNJS = getPluginConfig().MaxNJS.GetValue();
        filterOptions.minNPS = getPluginConfig().MinNPS.GetValue();
        filterOptions.maxNPS = getPluginConfig().MaxNPS.GetValue();
        filterOptions.rankedType = (FilterOptions::RankedFilterType) getPluginConfig().RankedType.GetValue();
        filterOptions.minStars = getPluginConfig().MinStars.GetValue();
        filterOptions.maxStars = getPluginConfig().MaxStars.GetValue();
        filterOptions.minUploadDate = getPluginConfig().MinUploadDate.GetValue();
        filterOptions.minRating = getPluginConfig().MinRating.GetValue();
        filterOptions.minVotes = getPluginConfig().MinVotes.GetValue();
        filterOptions.charFilter = (FilterOptions::CharFilterType) getPluginConfig().CharacteristicType.GetValue();
        filterOptions.difficultyFilter = (FilterOptions::DifficultyFilterType) getPluginConfig().DifficultyType.GetValue();
        filterOptions.modRequirement = (FilterOptions::RequirementType) getPluginConfig().RequirementType.GetValue();
        filterOptions.minUploadDateInMonths = getPluginConfig().MinUploadDateInMonths.GetValue();
        
        // Preferred Leaderboard
        std::string preferredLeaderboard = getPluginConfig().PreferredLeaderboard.GetValue();
        if (leaderBoardMap.contains(preferredLeaderboard)) {
            DataHolder::preferredLeaderboard = leaderBoardMap.at(preferredLeaderboard);
        } else {
            DataHolder::preferredLeaderboard = PreferredLeaderBoard::ScoreSaber;
            getPluginConfig().PreferredLeaderboard.SetValue("Scoresaber");
        }
        
        // Custom string loader
        auto uploadersString = getPluginConfig().Uploaders.GetValue();
        if (uploadersString.size() > 0) {
            if (uploadersString[0] == '!') {
                uploadersString.erase(0,1);
                filterOptions.uploadersBlackList = true;
            } else {
                filterOptions.uploadersBlackList = false;
            }
            filterOptions.uploaders = split(toLower(uploadersString), " ");
        } else {
            filterOptions.uploaders.clear();
        }
    }).detach();   
}

MAKE_AUTO_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    static bool debugstarted = false; 
    if (!debugstarted) {
        debugstarted = true;
        BSML::MainThreadScheduler::Schedule([]{
            coro(manager.Debug());
        });
    }
}

MAKE_HOOK_MATCH(ReturnToBSS, &HMUI::FlowCoordinator::DismissFlowCoordinator, void, HMUI::FlowCoordinator* self, HMUI::FlowCoordinator* flowCoordinator, HMUI::ViewController::AnimationDirection animationDirection, System::Action* finishedCallback, bool immediately) {
    if(!getPluginConfig().ReturnToBSS.GetValue()) {
        ReturnToBSS(self, flowCoordinator, animationDirection, finishedCallback, immediately);
        return;
    }
    if(!(flowCoordinator->get_gameObject()->get_name() == "SoloFreePlayFlowCoordinator")) {
        ReturnToBSS(self, flowCoordinator, animationDirection, finishedCallback, immediately);
        return;
    }

    ReturnToBSS(self, flowCoordinator, animationDirection, finishedCallback, true);
    if (fromBSS) {
        auto currentFlowCoordinator = BSML::Helpers::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
        auto betterSongSearchFlowCoordinator = UnityEngine::Resources::FindObjectsOfTypeAll<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>().FirstOrDefault();
        if(betterSongSearchFlowCoordinator)
            currentFlowCoordinator->PresentFlowCoordinator(betterSongSearchFlowCoordinator, nullptr, HMUI::ViewController::AnimationDirection::Horizontal, HMUI::ViewController::AnimationType::Out, false);
    }
    
};

MAKE_HOOK_MATCH(GameplaySetupViewController_RefreshContent, &GlobalNamespace::GameplaySetupViewController::RefreshContent, void, GlobalNamespace::GameplaySetupViewController* self)
{
    GameplaySetupViewController_RefreshContent(self);

    bool multiplayer = self->_showMultiplayer;

    // Button instance
    static SafePtrUnity<UnityEngine::GameObject> button;

    // Don't do anything if not in multiplayer to avoid messing with unity objects
    if (!multiplayer) {
        if (button) {
            button->set_active(multiplayer);
        }
        return;
    }
    

    if(!button) {
        DEBUG("Button not found, creating");
        auto x = self->get_transform()->Find("BSMLBackground/BSMLTabSelector");
        if (x==nullptr) {
            x = self->get_transform()->Find("TextSegmentedControl");
        }
        if (x == nullptr) {
            return;
        }

        button = UnityEngine::GameObject::Instantiate(x->get_transform()->GetChild(x->get_transform()->GetChildCount()-1), x)->get_gameObject();

        auto t = button->GetComponent<HMUI::TextSegmentedControlCell*>();

        t->set_text("Better Song Search");

        reinterpret_cast<HMUI::SelectableCell*>(t)->_wasPressedSignal=nullptr;

        
        std::function<void(HMUI::SelectableCell*, HMUI::SelectableCell::TransitionType, ::Il2CppObject*)> fun = [t](HMUI::SelectableCell* cell, HMUI::SelectableCell::TransitionType transition, Il2CppObject* obj) { 
            if(!t->get_selected())
                return;

            t->set_selected(false);
            manager.ShowFlow(false);
         };

        auto action = BSML::MakeSystemAction(fun);

        t->add_selectionDidChangeEvent(action);
    }

    button->set_active(multiplayer);
}

MAKE_HOOK_MATCH(LevelFilteringNavigationController_Setup, &GlobalNamespace::LevelFilteringNavigationController::Setup, void, GlobalNamespace::LevelFilteringNavigationController* self, GlobalNamespace::SongPackMask songPackMask, GlobalNamespace::IBeatmapLevelPack* levelPackToBeSelectedAfterPresent, GlobalNamespace::SelectLevelCategoryViewController::LevelCategory startLevelCategory, bool hidePacksIfOneOrNone, bool enableCustomLevels)
{
	LevelFilteringNavigationController_Setup(self, songPackMask, levelPackToBeSelectedAfterPresent, startLevelCategory, hidePacksIfOneOrNone, enableCustomLevels);

    // To have interoperability with pinkcore I trigger it only if pressing play in the bss
	if (openToCustom ) {
		self->_selectLevelCategoryViewController->Setup(startLevelCategory.CustomSongs, self->_enabledLevelCategories);
        openToCustom = false;
	}
}

MAKE_HOOK_MATCH(
    MultiplayerLevelScenesTransitionSetupDataSO_Init,
    &GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO::Init,
    void,
    GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO* self,
    StringW gameMode,
    GlobalNamespace::IPreviewBeatmapLevel *previewBeatmapLevel,
    GlobalNamespace::BeatmapDifficulty beatmapDifficulty,
    GlobalNamespace::BeatmapCharacteristicSO *beatmapCharacteristic,
    GlobalNamespace::IDifficultyBeatmap *difficultyBeatmap,
    GlobalNamespace::ColorScheme *overrideColorScheme,
    GlobalNamespace::GameplayModifiers *gameplayModifiers,
    GlobalNamespace::PlayerSpecificSettings *playerSpecificSettings,
    GlobalNamespace::PracticeSettings *practiceSettings,
    bool useTestNoteCutSoundEffects
) {
    // Close manager first
    manager.Close(true, false);
    MultiplayerLevelScenesTransitionSetupDataSO_Init(self, gameMode, previewBeatmapLevel, beatmapDifficulty, beatmapCharacteristic,difficultyBeatmap, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects );
}
	
// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    BSML::Init();

    INSTALL_HOOK(getLoggerOld(), ReturnToBSS);
    INSTALL_HOOK(getLoggerOld(), GameplaySetupViewController_RefreshContent);
    INSTALL_HOOK(getLoggerOld(), LevelFilteringNavigationController_Setup);
    // INSTALL_HOOK(getLoggerOld(), MainFlowCoordinator_DidActivate);
    INSTALL_HOOK(getLoggerOld(), MultiplayerLevelScenesTransitionSetupDataSO_Init);

    custom_types::Register::AutoRegister();

    modInfo.id = MOD_ID;
    
    manager.Init();
}