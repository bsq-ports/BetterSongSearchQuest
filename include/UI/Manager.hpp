#pragma once

#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/BSML.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

#include <vector>

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace GlobalNamespace;

namespace BetterSongSearch::UI {

    struct SongCacheEntry {
        GlobalNamespace::IBeatmapLevel* level;
        UnityEngine::Sprite* cover;
    };

    class Manager
    {
        HMUI::FlowCoordinator* parentFlow;
        SafePtrUnity<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator> flow;
        BSML::MenuButton * menuButton;
        

        public:
            std::vector<SongCacheEntry> coverCacheInvalidator;
            GlobalNamespace::IBeatmapLevel*  lastSelectedLevel = nullptr;
            Manager(Manager const&) = delete; // no accidental copying
            Manager() = default;

            void Init();

            custom_types::Helpers::Coroutine Debug();


            void ShowFlow(bool immediately);

            void GoToSongSelect();
    };


    inline static Manager manager;
}
