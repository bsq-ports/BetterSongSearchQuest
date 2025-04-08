#pragma once

#include "assets.hpp"
#include "bsml/shared/BSML.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/Touchable.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UI/ViewControllers/SongListCell.hpp"
#include "UnityEngine/UI/HorizontalOrVerticalLayoutGroup.hpp"

StringW const CustomSongListTableCellReuseIdentifier = "REUSECustomSongListTableCell";

namespace BetterSongSearch::UI::ViewControllers {
    class SongListTableData {
       public:
        static CustomSongListTableCell* GetCell(HMUI::TableView* tableView) {
            auto tableCell = tableView->DequeueReusableCellForIdentifier(CustomSongListTableCellReuseIdentifier);
            if (!tableCell) {
                //  old tableCell = UnityEngine::GameObject::New_ctor("CustomSongListTableCell", csTypeOf(HMUI::Touchable
                //  *))->AddComponent<CustomSongListTableCell *>();

                tableCell = UnityEngine::GameObject::New_ctor("CustomSongListTableCell")->AddComponent<CustomSongListTableCell*>();
                tableCell->_groupsAllowInteraction = true;  // needed for the cell to be interactable in 1.40.4
                tableCell->set_interactable(true);
                tableCell->set_reuseIdentifier(CustomSongListTableCellReuseIdentifier);
                BSML::parse_and_construct(Assets::SongListCell_bsml, tableCell->get_transform(), tableCell);

                // Weird hack cause HMUI touchable is not there for some reason, thanks RedBrumbler
                tableCell->get_gameObject()->AddComponent<HMUI::Touchable*>();

                auto cell = tableCell.cast<CustomSongListTableCell>();
                cell->diffs = cell->diffsContainer->GetComponentsInChildren<TMPro::TextMeshProUGUI*>();
            }

            return tableCell.cast<CustomSongListTableCell>();
        }
    };
}  // namespace BetterSongSearch::UI::ViewControllers
