#pragma once

#include "main.hpp"
#include "UI/ViewControllers/DownloadHistoryCell.hpp"
#include "HMUI/Touchable.hpp"
#include "bsml/shared/BSML.hpp"
#include "assets.hpp"

namespace BetterSongSearch::UI::ViewControllers {
    class DownloadListTableData {
        const std::string ReuseIdentifier = "REUSECustomDownloadListTableCell";

    public:
        CustomDownloadListTableCell* GetCell(HMUI::TableView* tableView) {
            auto tableCell = tableView->DequeueReusableCellForIdentifier(ReuseIdentifier);

            if(!tableCell) {
                tableCell = UnityEngine::GameObject::New_ctor("CustomDownloadListTableCell", csTypeOf(HMUI::Touchable*))->AddComponent<CustomDownloadListTableCell*>();
                tableCell->set_interactable(true);

                tableCell->set_reuseIdentifier(ReuseIdentifier);
                BSML::parse_and_construct(IncludedAssets::DownloadHistoryCell_bsml, tableCell->get_transform(), tableCell);
            }

            return reinterpret_cast<CustomDownloadListTableCell*>(tableCell);
        }
    };
}
