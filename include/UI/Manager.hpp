#pragma once

#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/BSML.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"


namespace BetterSongSearch::UI {
    class Manager
    {
        HMUI::FlowCoordinator* parentFlow;
        SafePtrUnity<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator> flow;
        SafePtrUnity<UnityEngine::GameObject> songSelectButton = nullptr;

        public:
            Manager(Manager const&) = delete; // no accidental copying
            Manager() = default;

            void Init(){
                BSML::Register::RegisterMenuButton("Better Song Search", "Search songs, but better", [this](){
                    DEBUG("MenuButtonClick");
                    ShowFlow(false);
                } );
            }

            

            void ShowFlow(bool immediately) {
                songSelectButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
                if (!songSelectButton) {
                    songSelectButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("Wrapper/BeatmapWithModifiers/BeatmapSelection/EditButton"));
                }
                // if (!songSelectButton) {
                //     return;
                // }
                
                DEBUG("Should create flow");
                if (!flow) {
                    DEBUG("CreateFlowCoordinator");
                    flow = BSML::Helpers::CreateFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>();
                }

                parentFlow = QuestUI::BeatSaberUI::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
                parentFlow->PresentFlowCoordinator(flow.ptr(), nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false);
            }

            void GoToSongSelect() {
                if (songSelectButton) {
                    auto event = songSelectButton->GetComponent<HMUI::NoTransitionsButton *>()->get_onClick();
                    event->Invoke();
                } else {
                    ERROR("no song select button");
                }
                
            }
    };


    inline static Manager manager;
}
