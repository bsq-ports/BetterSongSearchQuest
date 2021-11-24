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
#include "questui/shared/CustomTypes/Components/List/CustomCellListWrapper.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()
/*
// ):
static std::vector<Il2CppClass*> GetInterfaces() {
	return { classof(HMUI::TableView::IDataSource*) };
}

class DownloadHistoryData
{
    public:
    const SDC_wrapper::BeatStarSong* data;
    float progress;
}

___DECLARE_TYPE_WRAPPER_INHERITANCE(CustomComponents, DownloadHistoryListTableData, Il2CppTypeEnum::IL2CPP_TYPE_CLASS, UnityEngine::MonoBehaviour, "QuestUI", GetInterfaces(), 0, nullptr,
    DECLARE_INSTANCE_FIELD(Il2CppString*, cellTemplate);
    DECLARE_INSTANCE_FIELD(float, cellSize);
    DECLARE_INSTANCE_FIELD(HMUI::TableView*, tableView);
    DECLARE_INSTANCE_FIELD(bool, clickableCells);

    DECLARE_OVERRIDE_METHOD(HMUI::TableCell*, CellForIdx, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellForIdx>::get(), HMUI::TableView* tableView, int idx);
    DECLARE_OVERRIDE_METHOD(float, CellSize, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellSize>::get());
    DECLARE_OVERRIDE_METHOD(int, NumberOfCells, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::NumberOfCells>::get());

    public:
        QuestUI::CustomCellListWrapper* listWrapper = nullptr;
        std::vector<DownloadHistoryData*> data;
)

DECLARE_CLASS_CODEGEN(CustomComponents, DownloadHistoryCellTableCell, HMUI::TableCell,
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, mapperText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, uploadDateText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, ratingText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, songText);
    DECLARE_INSTANCE_FIELD(QuestUI::Backgroundable*, bg);

    DECLARE_CTOR(ctor);

    DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::SelectionDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::HighlightDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);

    DECLARE_INSTANCE_METHOD(void, RefreshVisuals);
    public:
    QuestUI::CustomTextSegmentedControlData* diffs;
    void RefreshData(DownloadHistoryData* data);
)*/

DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);
)