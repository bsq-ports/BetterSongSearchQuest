#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "HMUI/TableView_IDataSource.hpp"

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableCell.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()


DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController, HMUI::ViewController,
                      DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);
)
/*#pragma once

#include <shared/components/layouts/VerticalLayoutGroup.hpp>
#include <shared/components/layouts/HorizontalLayoutGroup.hpp>
#include "UnityEngine/MonoBehaviour.hpp"
#include "HMUI/TableView_IDataSource.hpp"

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableCell.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"

#include "questui_components/shared/components/Text.hpp"
#include "questui_components/shared/components/LoadingIndicator.hpp"
#include "questui_components/shared/components/Button.hpp"
#include "questui_components/shared/components/Image.hpp"
#include "questui_components/shared/reference_comp.hpp"


namespace BetterSongSearch::UI {
    struct DownloadListCell {
        QUC::Text songName{"SONGNAME", true, std::nullopt, 2.7, false};
        QUC::Text levelAuthorName{"AUTHOR", true, Sombrero::FastColor(0.8, 0.8, 0.8, 1.0), 2.3, false};
        QUC::Text statusLabel{"STATUS", true, Sombrero::FastColor::cyan(), 3, false};

        [[nodiscard]] auto VerticalLayout() {
            songName.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            songName.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            songName.wordWrapping = false;

            levelAuthorName.alignmentOptions = TMPro::TextAlignmentOptions::MidlineRight;
            levelAuthorName.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            levelAuthorName.wordWrapping = false;

            QUC::detail::HorizontalLayoutGroup songInfoLayout(
                QUC::detail::refComp(songName),
                QUC::detail::refComp(levelAuthorName)
            );

            songInfoLayout.spacing = 2;


            statusLabel.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            statusLabel.wordWrapping = false;

            QUC::detail::HorizontalLayoutGroup statusLayout(
                QUC::detail::refComp(statusLabel)
            );

            statusLayout

            QUC::detail::VerticalLayoutGroup verticalLayout(
                    songInfoLayout
            );

            modifyLayout.childForceExpandHeight = true;
            modifyLayout.padding = std::array<float, 4>{2, 2, 2, 2};

            QUC::ModifyLayoutElement layoutElement(modifyLayout);
            layoutElement.preferredWidth = 40;

            return layoutElement;
        }
    };

    struct DownloadHistory {
        QUC::Text title{"DOWNLOAD HISTORY", true, std::nullopt, 4, true};

    };
}*/