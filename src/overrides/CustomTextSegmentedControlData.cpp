#include "overrides/CustomTextSegmentedControlData.hpp"

#include <utility>

#include "UnityEngine/Object.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "HMUI/TextSegmentedControlCell.hpp"

DEFINE_TYPE(QUIOverride, CustomTextSegmentedControlData);

using namespace UnityEngine;

extern Logger& getLoggerOld();

namespace QUIOverride
{
    void CustomTextSegmentedControlData::ctor()
    {
        INVOKE_CTOR();
        texts = ArrayW<StringW>(static_cast<il2cpp_array_size_t>(0));
    }

    void CustomTextSegmentedControlData::dtor()
    {
        Finalize();
    }

    int CustomTextSegmentedControlData::NumberOfCells()
    {
        return texts.Length();
    }

    HMUI::SegmentedControlCell* CustomTextSegmentedControlData::CellForCellNumber(int idx)
    {
        HMUI::TextSegmentedControlCell* result = nullptr;
        if (texts.Length() == 1)
        {
            result = reinterpret_cast<HMUI::TextSegmentedControlCell*>(InstantiateCell(singleCellPrefab->get_gameObject()));
        }
        else if (idx == 0)
        {
            result = reinterpret_cast<HMUI::TextSegmentedControlCell*>(InstantiateCell(firstCellPrefab->get_gameObject()));
        } 
        else if (idx == texts.Length() - 1)
        {
            result = reinterpret_cast<HMUI::TextSegmentedControlCell*>(InstantiateCell(lastCellPrefab->get_gameObject()));
        }
        else
        {
            result = reinterpret_cast<HMUI::TextSegmentedControlCell*>(InstantiateCell(middleCellPrefab->get_gameObject()));
        }

        result->set_fontSize(fontSize);
        result->set_text(texts[idx]);
        result->set_hideBackgroundImage(hideCellBackground);

        if (overrideCellSize)
        {
            reinterpret_cast<RectTransform*>(result->get_transform())->set_sizeDelta({result->get_preferredWidth() + 2.0f * padding, 1.0f});
        }
        return result;
    }

    void CustomTextSegmentedControlData::set_texts(ArrayW<StringW> list)
    {
        texts = list;
        if (segmentedControl) segmentedControl->ReloadData();
    }

    void CustomTextSegmentedControlData::add_text(StringW addedText)
    {
        int len = texts.Length();
        ArrayW<StringW> newtexts(len);
        texts = newtexts;
        texts[len] = addedText;
        if (segmentedControl) segmentedControl->ReloadData();
    }

    HMUI::TextSegmentedControlCell* CustomTextSegmentedControlData::InstantiateCell(UnityEngine::GameObject* prefab)
    {
		auto gameObject = Object::Instantiate(prefab, {0, 0, 0}, Quaternion::get_identity(), get_transform());
		gameObject->get_transform()->set_localScale({1, 1, 1});
		return gameObject->GetComponent<HMUI::TextSegmentedControlCell*>();
    }

}