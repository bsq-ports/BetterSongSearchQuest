#pragma once

#include "main.hpp"
#include "UI/ViewControllers/SongListCell.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/TableView.hpp"
#include "bsml/shared/BSML.hpp"
#include "assets.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/UI/HorizontalOrVerticalLayoutGroup.hpp"

const StringW CustomSongListTableCellReuseIdentifier = "REUSECustomSongListTableCell";

namespace BetterSongSearch::UI::ViewControllers
{
    class SongListTableData
    {
    public:
        static CustomSongListTableCell *GetCell(HMUI::TableView *tableView)
        {
            auto tableCell = tableView->DequeueReusableCellForIdentifier(CustomSongListTableCellReuseIdentifier);
            if (!tableCell)
            {
                tableCell = UnityEngine::GameObject::New_ctor("CustomSongListTableCell", csTypeOf(HMUI::Touchable *))->AddComponent<CustomSongListTableCell *>();
                tableCell->set_interactable(true);
                tableCell->set_reuseIdentifier(CustomSongListTableCellReuseIdentifier);
                BSML::parse_and_construct(IncludedAssets::SongListCell_bsml, tableCell->get_transform(), tableCell);

                // Weird hack cause HMUI touchable is not there for some reason, thanks RedBrumbler
                tableCell->get_gameObject()->AddComponent<HMUI::Touchable *>();

                auto cell = reinterpret_cast<CustomSongListTableCell *>(tableCell);
                cell->diffs = cell->diffsContainer->GetComponentsInChildren<TMPro::TextMeshProUGUI*>();
            }

            return reinterpret_cast<CustomSongListTableCell *>(tableCell);
        }
    };
}
