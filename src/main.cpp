#include "main.hpp"
#include "_config.h"
#include "System/Action.hpp"
#include "custom-types/shared/register.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/BeatmapKey.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
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
#include "logging.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "DataHolder.hpp"

#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

using namespace BetterSongSearch::Util;
using namespace BetterSongSearch;
using namespace BetterSongSearch::UI;

// Called at the early stages of game loading
BSS_EXPORT_FUNC void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);

    getPluginConfig().Init(modInfo);

    INFO("Completed setup!");

    std::thread([]{
        INFO("setting config values");

        // Init the data holder (sub to events)
        dataHolder.Init();
    }).detach();   
}

MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    static bool debugstarted = false; 
    if (!debugstarted) {
        debugstarted = true;
        self->StartCoroutine(custom_types::Helpers::new_coro(
            manager.Debug()
        ));
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
        UnityW<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator> betterSongSearchFlowCoordinator = UnityEngine::Resources::FindObjectsOfTypeAll<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>()->FirstOrDefault();
        if(betterSongSearchFlowCoordinator)
            currentFlowCoordinator->PresentFlowCoordinator(betterSongSearchFlowCoordinator, nullptr, HMUI::ViewController::AnimationDirection::Horizontal, true, false);
    }
    
};

MAKE_HOOK_MATCH(GameplaySetupViewController_RefreshContent, &GlobalNamespace::GameplaySetupViewController::RefreshContent, void, GlobalNamespace::GameplaySetupViewController* self)
{
    GameplaySetupViewController_RefreshContent(self);

    bool multiplayer = self->____showMultiplayer;

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

        reinterpret_cast<HMUI::SelectableCell*>(t)->____wasPressedSignal=nullptr;

        
        std::function<void(UnityW<::HMUI::SelectableCell>, HMUI::SelectableCell::TransitionType, ::System::Object*)> fun = [t](UnityW<::HMUI::SelectableCell> cell, HMUI::SelectableCell::TransitionType transition, ::System::Object* obj) {
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

MAKE_HOOK_MATCH(
    LevelFilteringNavigationController_Setup, 
    &GlobalNamespace::LevelFilteringNavigationController::Setup, 
    void, 
    GlobalNamespace::LevelFilteringNavigationController* self, 
    GlobalNamespace::SongPackMask songPackMask,
    GlobalNamespace::BeatmapLevelPack* levelPackToBeSelectedAfterPresent,
    GlobalNamespace::__SelectLevelCategoryViewController__LevelCategory startLevelCategory,
    bool hidePacksIfOneOrNone,
    bool enableCustomLevels
)
{
	LevelFilteringNavigationController_Setup(self, songPackMask, levelPackToBeSelectedAfterPresent, startLevelCategory, hidePacksIfOneOrNone, enableCustomLevels);

    // To have interoperability with pinkcore I trigger it only if pressing play in the bss
	if (openToCustom ) {
		self->____selectLevelCategoryViewController->Setup(startLevelCategory.CustomSongs, self->____enabledLevelCategories);
        openToCustom = false;
	}
}

MAKE_HOOK_MATCH(
    MultiplayerLevelScenesTransitionSetupDataSO_Init,
    &GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO::Init,
    void,
    GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO* self,
    StringW gameMode,
    ByRef<::GlobalNamespace::BeatmapKey> beatmapKey,
    ::GlobalNamespace::BeatmapLevel* beatmapLevel,
    ::GlobalNamespace::IBeatmapLevelData* beatmapLevelData,
    ::GlobalNamespace::ColorScheme* overrideColorScheme,
    ::GlobalNamespace::GameplayModifiers* gameplayModifiers,
    ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings,
    ::GlobalNamespace::PracticeSettings* practiceSettings,
    ::GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader,
    ::BeatSaber::PerformancePresets::PerformancePreset* performancePreset,
    ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader,
    bool useTestNoteCutSoundEffects
) {
    // Close manager first
    manager.Close(true, false);
    MultiplayerLevelScenesTransitionSetupDataSO_Init(
            self,
            gameMode,
            beatmapKey,
            beatmapLevel,
            beatmapLevelData,
            overrideColorScheme,
            gameplayModifiers,
            playerSpecificSettings,
            practiceSettings,
            audioClipAsyncLoader,
            performancePreset,
            beatmapDataLoader,
            useTestNoteCutSoundEffects
            );
}
	
// Called later on in the game loading - a good time to install function hooks
BSS_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    BSML::Init();
    custom_types::Register::AutoRegister();

    INSTALL_HOOK(Logger, ReturnToBSS);
    INSTALL_HOOK(Logger, GameplaySetupViewController_RefreshContent);
    INSTALL_HOOK(Logger, LevelFilteringNavigationController_Setup);
    INSTALL_HOOK(Logger, MultiplayerLevelScenesTransitionSetupDataSO_Init);

    // Automatic testing
    // INSTALL_HOOK(Logger, MainFlowCoordinator_DidActivate);

    manager.Init();
}