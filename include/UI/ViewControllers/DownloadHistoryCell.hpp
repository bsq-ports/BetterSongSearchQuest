#pragma once

#include "DownloadHistory.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_IDataSource.hpp"
#include "HMUI/TableCell.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "song-details/shared/Data/Song.hpp"
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()


DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, CustomDownloadListTableCell, HMUI::TableCell,
    DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, GET_FIND_METHOD(&HMUI::SelectableCell::SelectionDidChange), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, GET_FIND_METHOD(&HMUI::SelectableCell::HighlightDidChange), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, WasPreparedForReuse, GET_FIND_METHOD(&HMUI::TableCell::WasPreparedForReuse));
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, bgContainer);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, bgProgress);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, songName);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, levelAuthorName);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, statusLabel);

public:
    DownloadHistoryEntry* entry;

    CustomDownloadListTableCell* PopulateWithSongData(DownloadHistoryEntry* entry);
    void UpdateProgress();
    void RefreshBar();
    void RefreshBgState();
)