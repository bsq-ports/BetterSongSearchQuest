#include "main.hpp"

#include "questui/shared/QuestUI.hpp"
#include "System/Action.hpp"
#include "System/Func_1.hpp"
#include "System/Func_2.hpp"
#include "System/Action_1.hpp"
#include "custom-types/shared/register.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "HMUI/SelectableCell.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/GameplaySetupViewController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/ISpriteAsyncLoader.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/IO/Path.hpp"
#include "System/IO/File.hpp"

#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "PluginConfig.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "System/Threading/Tasks/TaskCanceledException.hpp"

#include "UnityEngine/Texture2D.hpp"
#include "UI/Manager.hpp"
#include "HMUI/TextSegmentedControlCell.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "Util/TextUtil.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"

#include <regex>
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

using namespace QuestUI;
using namespace GlobalNamespace;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

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

MAKE_HOOK_MATCH(StandardLevelDetailView_SetContent, &StandardLevelDetailView::SetContent, void, StandardLevelDetailView* self, IBeatmapLevel* level, BeatmapDifficulty defaultDifficulty, BeatmapCharacteristicSO* defaultBeatmapCharacteristic, PlayerData* playerData) {
    // Prefix
    // fix
    StandardLevelDetailView_SetContent(self, level, defaultDifficulty, defaultBeatmapCharacteristic, playerData);
    // postfix
    manager.lastSelectedLevel = level;
};

MAKE_HOOK_MATCH(CustomPreviewBeatmapLevel_GetCoverImageAsync, &CustomPreviewBeatmapLevel::GetCoverImageAsync, System::Threading::Tasks::Task_1<UnityEngine::Sprite *>*, CustomPreviewBeatmapLevel* self, System::Threading::CancellationToken cancellationToken) {
    static int MAX_CACHED_COVERS = 20;
    if (self->coverImage != nullptr && self->coverImage->m_CachedPtr.m_value != nullptr) {
        int cachedIndex = -1;
        // "Refresh" the cover in the cache LIFO
        for (auto i=0; i< manager.coverCacheInvalidator.size(); i--) {
            auto level = reinterpret_cast<CustomPreviewBeatmapLevel*>(manager.coverCacheInvalidator[i].level);
            if (level == self) {
                cachedIndex = i;
                break;
            } 
        }

        // Move to top
        if (cachedIndex != 1 && cachedIndex + 1 != manager.coverCacheInvalidator.size()) {
            manager.coverCacheInvalidator.push_back(manager.coverCacheInvalidator[cachedIndex]);
            manager.coverCacheInvalidator.erase(manager.coverCacheInvalidator.begin()+cachedIndex);
        }

        DEBUG("Cached image");
        return System::Threading::Tasks::Task_1<UnityEngine::Sprite *>::FromResult(self->coverImage);
    }

    if (System::String::IsNullOrEmpty(self->standardLevelInfoSaveData->coverImageFilename)) {
        DEBUG("Default image");
        return System::Threading::Tasks::Task_1<UnityEngine::Sprite *>::FromResult(self->defaultCoverImage);
    }

    StringW path = System::IO::Path::Combine(self->customLevelPath, self->standardLevelInfoSaveData->coverImageFilename);

    if(!System::IO::File::Exists(path)) {
        DEBUG("File does not exist");
        return System::Threading::Tasks::Task_1<UnityEngine::Sprite *>::FromResult(self->defaultCoverImage);
    }

    if (cancellationToken.get_IsCancellationRequested()) {
        DEBUG("Fix tis");
        // return System::Threading::Tasks::Task_1<UnityEngine::Sprite *>::FromException(System::Exception::New_ctor("Cancelled"));
    }
    
    using Task = System::Threading::Tasks::Task_1<UnityEngine::Sprite*>*;
    using Action = System::Func_2<Task, UnityEngine::Sprite*>*;

    auto middleware = custom_types::MakeDelegate<Action>(classof(Action), static_cast<std::function<UnityEngine::Sprite* (Task)>>([self](Task resultTask) {
        bool cancelled = resultTask->get_IsCanceled();
        if (cancelled) {
            DEBUG("Task cancelled");
            return nullptr;
        }
        UnityEngine::Sprite* cover = resultTask->get_ResultOnSuccess();
        if (cover != nullptr && cover->m_CachedPtr.m_value != nullptr) {
            self->coverImage = cover;
            manager.coverCacheInvalidator.push_back({
                reinterpret_cast<GlobalNamespace::IBeatmapLevel*>(self), self->coverImage
            });

            QuestUI::MainThreadScheduler::Schedule([]{
                for(int i = manager.coverCacheInvalidator.size() - MAX_CACHED_COVERS; i-- > 0;) {

                auto songToInvalidate = manager.coverCacheInvalidator[i];
                // Skip selected level
                if(manager.lastSelectedLevel == songToInvalidate.level)
            		continue;
                manager.coverCacheInvalidator.erase(manager.coverCacheInvalidator.begin()+i);
                if(songToInvalidate.level != nullptr && songToInvalidate.cover != nullptr && songToInvalidate.cover->m_CachedPtr.m_value != nullptr) {
                    auto song = reinterpret_cast<CustomPreviewBeatmapLevel*>(songToInvalidate.level);
                    song->coverImage = nullptr;
                    DEBUG("DeletingCover {}", std::string(song->songName) );
                    if (songToInvalidate.cover != nullptr && songToInvalidate.cover->get_texture() != nullptr) {
                    UnityEngine::Object::DestroyImmediate(songToInvalidate.cover->get_texture());    
                    }
                    if (songToInvalidate.cover != nullptr) {
                        UnityEngine::Object::DestroyImmediate(songToInvalidate.cover);
                    }
                    
                }
            }     
            });
        } else {
            DEBUG("No cover found");
        }
        return nullptr;
    }));
    // self->get_spriteAsyncLoader()->LoadSpriteAsync(path, cancellationToken);


// System.Threading.Tasks.Task<TNewResult> ContinueWith(System.Func<System.Threading.Tasks.Task<UnityEngine.Sprite>,TNewResult> continuationFunction);
// System.Threading.Tasks.Task<TNewResult> ContinueWith(System.Func<System.Threading.Tasks.Task<UnityEngine.Sprite>,TNewResult> continuationFunction, System.Threading.Tasks.TaskScheduler scheduler, System.Threading.CancellationToken cancellationToken, System.Threading.Tasks.TaskContinuationOptions continuationOptions, out/ref System.Threading.StackCrawlMark& stackMark);


    auto lol = self->get_spriteAsyncLoader()->LoadSpriteAsync(path, System::Threading::CancellationToken::get_None());

    static auto ___internal__logger = ::Logger::get().WithContext("::System::Threading::Tasks::Task_1::ContinueWith");
    // static auto* ___internal__method = THROW_UNLESS((::il2cpp_utils::FindMethod(lol, "ContinueWith", std::vector<Il2CppClass*>{::il2cpp_utils::il2cpp_type_check::il2cpp_gen_struct_no_arg_class<UnityEngine::Sprite*>::get()}, ::std::vector<const Il2CppType*>{::il2cpp_utils::ExtractType(middleware)})));
    static auto* ___internal__method = ::il2cpp_utils::FindMethodUnsafe(lol, "ContinueWith", 1);
    static auto* ___generic__method = THROW_UNLESS(::il2cpp_utils::MakeGenericMethod(___internal__method, std::vector<Il2CppClass*>{::il2cpp_utils::il2cpp_type_check::il2cpp_no_arg_class<UnityEngine::Sprite*>::get()}));
    return ::il2cpp_utils::RunMethodRethrow<::System::Threading::Tasks::Task_1<UnityEngine::Sprite*>*, false>(lol, ___generic__method, middleware);



    // return lol->ContinueWith(action);
    // return CustomPreviewBeatmapLevel_GetCoverImageAsync(self, cancellationToken);
    // postfix
   
};



// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    getPluginConfig().Init(info);
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

// MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
//     MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
//     static bool debugstarted = false; 
//     // if (!debugstarted) {
//     //     debugstarted = true;
//     //     QuestUI::MainThreadScheduler::Schedule([]{
//     //         coro(manager.Debug());
//     //     });
//     // }
// }

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
        auto currentFlowCoordinator = QuestUI::BeatSaberUI::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
        auto betterSongSearchFlowCoordinator = UnityEngine::Resources::FindObjectsOfTypeAll<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>().FirstOrDefault();
        if(betterSongSearchFlowCoordinator)
            currentFlowCoordinator->PresentFlowCoordinator(betterSongSearchFlowCoordinator, nullptr, HMUI::ViewController::AnimationDirection::Horizontal, HMUI::ViewController::AnimationType::Out, false);
    }
    
}

MAKE_HOOK_MATCH(GameplaySetupViewController_RefreshContent, &GlobalNamespace::GameplaySetupViewController::RefreshContent, void, GlobalNamespace::GameplaySetupViewController* self)
{
    GameplaySetupViewController_RefreshContent(self);

    // FIXME: This button does not get recreated in the multiplayer menu for some reason, find a way to detect button dying
    // Button instance
    static SafePtrUnity<UnityEngine::GameObject> button;

    bool multiplayer = self->showMultiplayer;

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

        reinterpret_cast<HMUI::SelectableCell*>(t)->wasPressedSignal=nullptr;

        
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
		self->selectLevelCategoryViewController->Setup(startLevelCategory.CustomSongs, self->enabledLevelCategories);
        openToCustom = false;
	}
}
	
// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    QuestUI::Init();

    INSTALL_HOOK(getLoggerOld(), ReturnToBSS);
    INSTALL_HOOK(getLoggerOld(), GameplaySetupViewController_RefreshContent);
    INSTALL_HOOK(getLoggerOld(), LevelFilteringNavigationController_Setup);
    INSTALL_HOOK(getLoggerOld(), StandardLevelDetailView_SetContent);
    INSTALL_HOOK(getLoggerOld(), CustomPreviewBeatmapLevel_GetCoverImageAsync);
    // INSTALL_HOOK(getLoggerOld(), MainFlowCoordinator_DidActivate);
    

    custom_types::Register::AutoRegister();

    modInfo.id = MOD_ID;

    manager.Init();
}