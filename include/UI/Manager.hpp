#pragma once

#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/BSML.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace GlobalNamespace;

namespace BetterSongSearch::UI {
    class Manager
    {
        HMUI::FlowCoordinator* parentFlow;
        SafePtrUnity<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator> flow;
        BSML::MenuButton * menuButton;
        

        public:
            bool inMultiplayer = false;
            Manager(Manager const&) = delete; // no accidental copying
            Manager() = default;

            void Init();

            custom_types::Helpers::Coroutine Debug();


            void ShowFlow(bool immediately, bool multiplayer = false);

            void Close(bool immediately = false, bool downloadAbortConfim = true);

            void GoToSongSelect();
    };


    inline static Manager manager;
}
