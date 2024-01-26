#include "UI/Manager.hpp"

#include <bsml/shared/Helpers/getters.hpp>

#include "Util/Debug.hpp"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "System/GC.hpp"
#include "UnityEngine/Profiling/Profiler.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/NoTransitionsButton.hpp"


using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace GlobalNamespace;

#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

  
void BetterSongSearch::UI::Manager::Init(){
    BSML::Register::RegisterMenuButton("Better Song Search", "Search songs, but better", [this](){
        DEBUG("MenuButtonClick");
        ShowFlow(false);
    } );
}

custom_types::Helpers::Coroutine BetterSongSearch::UI::Manager::Debug() {
    static struct sysinfo memInfo;
    int iterations_count = 0;
    int64_t lastused = 0;
    int64_t initialmemusage = UnityEngine::Profiling::Profiler::GetMonoUsedSizeLong();
    // wait for the game to start
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(10.0f));

    // Open the BSS
    ShowFlow(true);
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(1.0f));

    // Wait for songs to load
    while (!DataHolder::loaded) {
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.2f));
    }

    co_return;

    while(true) {
        
       
        // Tests for the fcinstance
        if (fcInstance != nullptr && fcInstance->m_CachedPtr != nullptr) {
        //     // Select random song go into the play menu and go back
        //     {
        //         auto* songlist = fcInstance->SongListController;
        //         co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.5f));
        //         songlist->SelectRandom();
        //         if (songlist->currentSong) {
        //             co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(1.0f));

        //             if (songlist->playButton->get_gameObject()->get_active()) {
        //                 songlist->PlaySong(songlist->currentSong);
        //                 co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(2.5f));
        //                 // Press back button
        //                 UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("BackButton"))->GetComponent<HMUI::NoTransitionsButton*>()->Press();
        //             }
                    
                    

                   
                    
        //             ++iterations_count;
        //         }
        //     }

            // Pick random song, show details, close details
            {
                auto* songlist = fcInstance->SongListController;
                co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.1f));
                songlist->SelectRandom();
                if (songlist->currentSong) {
                    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.1f));
                    songlist->ShowSongDetails();
                    co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.1f));
                    songlist->uploadDetailsModal->CloseModal();
                    ++iterations_count;
                }
            }




        }
        
        
        // Debug restart of the game
        // {   
        //     DEBUG("Open BSS");
            

            

        //     DEBUG("Press back button");
        //     // Press back button
        //     UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("BackButton"))->GetComponent<HMUI::NoTransitionsButton*>()->Press();

        //     co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(3.0f));


        //     DEBUG("Reload the game manually");
        //     // Reload the game manually
        //     UnityEngine::Resources::FindObjectsOfTypeAll<MenuTransitionsHelper*>()[0]->RestartGame(nullptr);
        //     co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(5.0f));
        // }

        // Select current song, go into song list and back
        // {
        //     co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.5f));
        //     // this->SelectRandom();
        //     if (this->currentSong) {
        //         co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.5f));
        //         this->PlaySong(this->currentSong);
        //         co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(1.5f));

        //         // Press back button
        //         UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("BackButton"))->GetComponent<HMUI::NoTransitionsButton*>()->Press();
        //         // manager.ShowFlow(true);
                
        //         ++iterations_count;
        //     }
        // }
        

        // Collect garbage
        System::GC::Collect();

        int64_t mem = UnityEngine::Profiling::Profiler::GetMonoUsedSizeLong();
        DEBUG("\nIter {}\nUsed {}\nChan {}\nInit: {}", iterations_count,  pretty_bytes(mem), pretty_bytes(mem - lastused), pretty_bytes(lastused - initialmemusage));
        lastused = mem;


        
    }
    co_yield nullptr;
}


void BetterSongSearch::UI::Manager::ShowFlow(bool immediately) {
    if (!flow) {
        flow = BSML::Helpers::CreateFlowCoordinator<BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator*>();
    }
    parentFlow = BSML::Helpers::GetMainFlowCoordinator()->YoungestChildFlowCoordinatorOrSelf();
    parentFlow->PresentFlowCoordinator(flow.ptr(), nullptr, HMUI::ViewController::AnimationDirection::Horizontal, false, false);
}

void BetterSongSearch::UI::Manager::GoToSongSelect() {
    SafePtrUnity<UnityEngine::GameObject> songSelectButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("SoloButton"));
    if (!songSelectButton) {
        songSelectButton = UnityEngine::GameObject::Find(il2cpp_utils::newcsstr("Wrapper/BeatmapWithModifiers/BeatmapSelection/EditButton"));
    }
    if (!songSelectButton) {
        return;
    }
    songSelectButton->GetComponent<HMUI::NoTransitionsButton *>()->Press();
}

void BetterSongSearch::UI::Manager::Close(bool immediately, bool downloadAbortConfim) {
    if (flow) {
        flow->Close(immediately, downloadAbortConfim);
    }
};