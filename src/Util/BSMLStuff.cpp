#include "Util/BSMLStuff.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/EventSystemListener.hpp"
#include "HMUI/CustomFormatRangeValuesSlider.hpp"
#include "HMUI/VerticalScrollIndicator.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UnityEngine/Coroutine.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "main.hpp"
#include "custom-types/shared/coroutine.hpp"

#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

namespace BetterSongSearch::UI::Util::BSMLStuff
{
    custom_types::Helpers::Coroutine MergeSliders(GameObject* container, bool constrictValuesMinMax)
    {
        co_yield nullptr;
        auto items = container->GetComponentsInChildren<HMUI::CurvedTextMeshPro *>();
     
        // Construct a list of MERGE_TO_PREV
        std::vector<HMUI::CurvedTextMeshPro *> filteredItems = std::vector<HMUI::CurvedTextMeshPro *>();

        for (auto item : items)
        {
            if (item != nullptr && item->m_CachedPtr.m_value != nullptr && item->get_text() == "MERGE_TO_PREV")
            {
                filteredItems.push_back(item);
            }
        }
        
        for (auto x : filteredItems) {
            co_yield reinterpret_cast<System::Collections::IEnumerator*>(WaitForEndOfFrame::New_ctor());
            auto ourContainer = x->get_transform()->get_parent();
            auto prevContainer = ourContainer->get_parent()->GetChild(ourContainer->GetSiblingIndex() - 1);

            reinterpret_cast<UnityEngine::RectTransform*> (prevContainer->Find("BSMLSlider"))->set_offsetMax(Vector2(-20, 0));
            reinterpret_cast<UnityEngine::RectTransform*> (ourContainer->Find("BSMLSlider"))->set_offsetMin(Vector2(-20, 0));
            ourContainer->set_position(prevContainer->get_position());

            auto minTimeSlider = prevContainer->GetComponentInChildren<HMUI::CustomFormatRangeValuesSlider*>();
            auto maxTimeSlider = ourContainer->GetComponentInChildren<HMUI::CustomFormatRangeValuesSlider*>();

            if(minTimeSlider == nullptr || maxTimeSlider == nullptr) {
                co_yield nullptr;
            } else {
                auto newSize = minTimeSlider->get_valueSize() / 2.1f;
                maxTimeSlider->set_valueSize(newSize);
                minTimeSlider->set_valueSize(newSize);
                ourContainer->GetComponentInChildren<UnityEngine::UI::LayoutElement*>()->set_ignoreLayout(true);
                x->set_text("");
            }
        }
    }
};




