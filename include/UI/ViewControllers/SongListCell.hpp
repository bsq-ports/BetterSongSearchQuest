#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
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
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "HMUI/ImageView.hpp"

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell, HMUI::TableCell,
    DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, GET_FIND_METHOD(&HMUI::SelectableCell::SelectionDidChange), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, GET_FIND_METHOD(&HMUI::SelectableCell::HighlightDidChange), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, WasPreparedForReuse, GET_FIND_METHOD(&HMUI::TableCell::WasPreparedForReuse));
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, fullFormattedSongName);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, uploadDateFormatted);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, levelAuthorName);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, songLengthAndRating);
    DECLARE_INSTANCE_FIELD(ArrayW<TMPro::TextMeshProUGUI*>, diffs);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, diffsContainer);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView *, bgContainer);


public:
    CustomSongListTableCell* PopulateWithSongData(const SDC_wrapper::BeatStarSong* entry);
    const SDC_wrapper::BeatStarSong* entry;

    
    void SetFontSizes();
    void RefreshBgState();
)