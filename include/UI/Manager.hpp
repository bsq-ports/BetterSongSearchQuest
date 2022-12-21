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
        BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator* flow;
        // static UnityEngine::UI::Button::ButtonClickedEvent* goToSongSelect = nullptr;

        public:
            Manager(Manager const&) = delete; // no accidental copying
            Manager() = default;

            void Init(){
                BSML::Register::RegisterMenuButton("Better Song Search", "Search songs, but better", [this](){
                    ShowFlow(false);
                } );
            }

            

            void ShowFlow(bool immediately) {
                
                if (flow == nullptr || flow->m_CachedPtr.m_value == nullptr) {
                    flow = BSML::Helpers::CreateFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>();
                }

                parentFlow = QuestUI::BeatSaberUI::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
                parentFlow->PresentFlowCoordinator(flow, nullptr, HMUI::ViewController::AnimationDirection::Horizontal, HMUI::ViewController::AnimationType::Out, false);
            }

            void GoToSongSelect() {
                auto button = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
                if (button == nullptr || button->m_CachedPtr.m_value == nullptr) {
                    button = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("Wrapper/BeatmapWithModifiers/BeatmapSelection/EditButton"));
                }
                if (button == nullptr || button->m_CachedPtr.m_value == nullptr) {
                    return;
                }
                button->GetComponent<HMUI::NoTransitionsButton *>()->Press();
            }
    };


    inline static Manager manager;
}
