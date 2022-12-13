#pragma once

#include "main.hpp"
#include "UI/ViewControllers/DownloadHistoryCell.hpp"
#include "HMUI/Touchable.hpp"
#include "bsml/shared/BSML.hpp"
#include "assets.hpp"

const StringW ReuseIdentifier = "REUSECustomDownloadListTableCell";

namespace BetterSongSearch::UI::ViewControllers
{
    class DownloadListTableData
    {
    public:
        static CustomDownloadListTableCell *GetCell(HMUI::TableView *tableView)
        {
            auto tableCell = tableView->DequeueReusableCellForIdentifier(ReuseIdentifier);
            if (!tableCell)
            {
                tableCell = UnityEngine::GameObject::New_ctor("CustomDownloadListTableCell", csTypeOf(HMUI::Touchable *))->AddComponent<CustomDownloadListTableCell *>();
                tableCell->set_interactable(true);
                tableCell->set_reuseIdentifier(ReuseIdentifier);
                BSML::parse_and_construct(IncludedAssets::DownloadHistoryCell_bsml, tableCell->get_transform(), tableCell);
                
                // Weird hack cause HMUI touchable is not there for some reason
                tableCell->get_gameObject()->AddComponent<HMUI::Touchable *>();
            }

            return reinterpret_cast<CustomDownloadListTableCell *>(tableCell);
        }
    };
}
