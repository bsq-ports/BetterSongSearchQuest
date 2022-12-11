#include "Util/BSMLStuff.hpp"
#include "main.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "HMUI/CustomFormatRangeValuesSlider.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"

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

    // TODO: Port these to fix scrollbar
    // static GameObject scrollBar = null;

	// 	public static GameObject GetScrollbarForTable(GameObject table, Transform targetContainer) {
	// 		if(scrollBar == null)
	// 			scrollBar = Resources.FindObjectsOfTypeAll<VerticalScrollIndicator>().FirstOrDefault(x => x.enabled)?.transform.parent?.gameObject;

	// 		if(scrollBar == null)
	// 			return null;

	// 		var sw = table.GetComponentInChildren<ScrollView>();

	// 		if(sw == null)
	// 			return null;

	// 		var listScrollBar = GameObject.Instantiate(scrollBar, targetContainer, false);
	// 		listScrollBar.SetActive(true);
	// 		var vsi = listScrollBar.GetComponentInChildren<VerticalScrollIndicator>(true);

	// 		ReflectionUtil.SetField(sw, "_verticalScrollIndicator", vsi);

	// 		var buttoneZ = listScrollBar.GetComponentsInChildren<NoTransitionsButton>(true).OrderByDescending(x => x.gameObject.name == "UpButton").ToArray();
	// 		if(buttoneZ.Length == 2) {
	// 			ReflectionUtil.SetField(sw, "_pageUpButton", (Button)buttoneZ[0]);
	// 			ReflectionUtil.SetField(sw, "_pageDownButton", (Button)buttoneZ[1]);

	// 			buttoneZ[0].onClick.AddListener(sw.PageUpButtonPressed);
	// 			buttoneZ[1].onClick.AddListener(sw.PageDownButtonPressed);
	// 		}

	// 		// I dont know WHY I need do do this, but if I dont the scrollbar wont work with the added modal.
	// 		foreach(Transform x in listScrollBar.transform) {
	// 			foreach(var y in x.GetComponents<Behaviour>())
	// 				y.enabled = true;
	// 		}

	// 		sw.Update();
	// 		sw.gameObject.AddComponent<RefreshScrolbarOnFirstLoad>();

	// 		return scrollBar;
	// 	}

	// 	class RefreshScrolbarOnFirstLoad : MonoBehaviour {
	// 		void OnEnable() => StartCoroutine(dorefresh());

	// 		IEnumerator dorefresh() {
	// 			yield return null;
	// 			var sv = gameObject.GetComponent<ScrollView>();

	// 			if(sv == null)
	// 				yield break;
	// 			ReflectionUtil.GetField<VerticalScrollIndicator, ScrollView>(sv, "_verticalScrollIndicator")?.RefreshHandle();
	// 		}
	// 	}
}
