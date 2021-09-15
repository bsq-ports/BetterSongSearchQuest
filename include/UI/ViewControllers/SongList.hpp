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
#include "../Sprites.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"

extern std::vector<const SDC_wrapper::BeatStarSong*> songList;
extern std::vector<const SDC_wrapper::BeatStarSong*> filteredSongList;
#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

// ):
static std::vector<Il2CppClass*> GetInterfaces() {
	return { classof(HMUI::TableView::IDataSource*) };
}

DECLARE_CLASS_CODEGEN(CustomComponents, SongListCellData, System::Object,
    DECLARE_INSTANCE_FIELD(Il2CppString*, songName);
    DECLARE_INSTANCE_FIELD(Il2CppString*, author);
    DECLARE_INSTANCE_FIELD(Il2CppString*, mapper);
    DECLARE_CTOR(ctor, Il2CppString* _songName, Il2CppString* _author, Il2CppString* _mapper);
)

___DECLARE_TYPE_WRAPPER_INHERITANCE(CustomComponents, CustomCellListTableData, Il2CppTypeEnum::IL2CPP_TYPE_CLASS, UnityEngine::MonoBehaviour, "QuestUI", GetInterfaces(), 0, nullptr,
    DECLARE_INSTANCE_FIELD(Il2CppString*, cellTemplate);
    DECLARE_INSTANCE_FIELD(float, cellSize);
    DECLARE_INSTANCE_FIELD(HMUI::TableView*, tableView);
    DECLARE_INSTANCE_FIELD(bool, clickableCells);

    //DECLARE_CTOR(ctor);
    //DECLARE_DTOR(dtor);

    DECLARE_OVERRIDE_METHOD(HMUI::TableCell*, CellForIdx, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellForIdx>::get(), HMUI::TableView* tableView, int idx);
    DECLARE_OVERRIDE_METHOD(float, CellSize, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellSize>::get());
    DECLARE_OVERRIDE_METHOD(int, NumberOfCells, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::NumberOfCells>::get());

    public:
        QuestUI::CustomCellListWrapper* listWrapper = nullptr;
        std::vector<const SDC_wrapper::BeatStarSong*> data;
)

DECLARE_CLASS_CODEGEN(CustomComponents, SongListCellTableCell, HMUI::TableCell,
    DECLARE_INSTANCE_FIELD(List<UnityEngine::GameObject*>*, selectedGroup);
    DECLARE_INSTANCE_FIELD(List<UnityEngine::GameObject*>*, hoveredGroup);
    DECLARE_INSTANCE_FIELD(List<UnityEngine::GameObject*>*, neitherGroup);

    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, mapperText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, uploadDateText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, ratingText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, songText);
    DECLARE_INSTANCE_FIELD(QuestUI::Backgroundable*, bg);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, diffsGroup);

    DECLARE_CTOR(ctor);

    DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::SelectionDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::HighlightDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);

    DECLARE_INSTANCE_METHOD(void, RefreshVisuals);
    public:
    std::vector<TMPro::TextMeshProUGUI*> texts;
    void RefreshData(const SDC_wrapper::BeatStarSong* data);
)

DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, SongListViewController, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);
)
static CustomComponents::CustomCellListTableData* tableData;