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
        // static UnityEngine::UI::Button::ButtonClickedEvent* goToSongSelect = nullptr;

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
                DEBUG("Should create flow");
                if (!flow) {
                    DEBUG("CreateFlowCoordinator");
                    flow = BSML::Helpers::CreateFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>();
                }

                parentFlow = QuestUI::BeatSaberUI::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
                parentFlow->PresentFlowCoordinator(flow.ptr(), nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false);
            }

            void GoToSongSelect() {
                SafePtrUnity<UnityEngine::GameObject> button = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
                if (!button) {
                    button = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("Wrapper/BeatmapWithModifiers/BeatmapSelection/EditButton"));
                }
                if (!button) {
                    return;
                }
                button->GetComponent<HMUI::NoTransitionsButton *>()->Press();
            }
    };


    inline static Manager manager;
}
