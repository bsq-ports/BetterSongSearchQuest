#include "UI/Modals/PresetsTable.hpp"
#include "sombrero/shared/FastColor.hpp"

DEFINE_TYPE(BetterSongSearch::UI::Modals, PresetsTableCell);

const StringW PresetsTableCellReuseIdentifier = "REUSEPresetsTableCell";

namespace BetterSongSearch::UI::Modals
{
    void PresetsTableCell::RefreshBgState()
    {
        static auto hovered = Sombrero::FastColor(1.0f, 1.0f, 1.0f, 1.0f);
		static auto normal = Sombrero::FastColor(0.0f, 0.0f, 0.0f, 0.45f);
        bgContainer->set_color((selected || highlighted) ? hovered : normal);
    }

    void PresetsTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType)
    {
        RefreshBgState();
    }

    void PresetsTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType)
    {
        RefreshBgState();
    }

    void PresetsTableCell::WasPreparedForReuse()
    {
        presetNameLabel->set_text("");
    }


    PresetsTableCell* PresetsTableCell::PopulateWithPresetName(StringW presetName) {
        presetNameLabel->set_text(presetName);
        return this;
    }

    PresetsTableCell *PresetsTableCell::GetCell(HMUI::TableView *tableView)
    {
        auto tableCell = tableView->DequeueReusableCellForIdentifier(PresetsTableCellReuseIdentifier);
        if (!tableCell)
        {
            tableCell = UnityEngine::GameObject::New_ctor("PresetsTableCell")->AddComponent<PresetsTableCell *>();
            tableCell->set_interactable(true);
            tableCell->set_reuseIdentifier(PresetsTableCellReuseIdentifier);
            BSML::parse_and_construct(Assets::PresetsListCell_bsml, tableCell->get_transform(), tableCell);

            // Weird hack cause HMUI touchable is not there for some reason, thanks RedBrumbler
            tableCell->get_gameObject()->AddComponent<HMUI::Touchable *>();
        }

        return tableCell.cast<PresetsTableCell>();
    }
}
