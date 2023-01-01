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
                if (!flow) {
                    flow = BSML::Helpers::CreateFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>();
                }

                parentFlow = QuestUI::BeatSaberUI::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
                parentFlow->PresentFlowCoordinator(flow.ptr(), nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false);
            }

            void GoToSongSelect() {
                SafePtrUnity<UnityEngine::GameObject> songSelectButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
                if (!songSelectButton) {
                    songSelectButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("Wrapper/BeatmapWithModifiers/BeatmapSelection/EditButton"));
                }
                if (!songSelectButton) {
                    return;
                }
                songSelectButton->GetComponent<HMUI::NoTransitionsButton *>()->Press();
            }
    };


    inline static Manager manager;
}
