#pragma once
#include "UI/ViewControllers/SongListCellTableData.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "HMUI/TableView_IDataSource.hpp"
#include "HMUI/TableView.hpp"
#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableCell.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/TextPageScrollView.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"
#include "bsml/shared/BSML/ViewControllers/HotReloadViewController.hpp"
#include "questui/shared/CustomTypes/Components/SegmentedControl/CustomTextSegmentedControlData.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "overrides/CustomSegmentedControl.hpp"
#include "songloader/shared/API.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "main.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "Util/RatelimitCoroutine.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"
#include "UI/SelectedSongController.hpp"
#include "UnityEngine/UI/HorizontalOrVerticalLayoutGroup.hpp"


#include "FilterOptions.hpp"
#include <fmt/chrono.h>
#ifndef DECLARE_OVERRIDE_METHOD_MATCH
#define DECLARE_OVERRIDE_METHOD_MATCH(retval, method, mptr, ...) \
    DECLARE_OVERRIDE_METHOD(retval, method, il2cpp_utils::il2cpp_type_check::MetadataGetter<mptr>::get(), __VA_ARGS__)
#endif


#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

namespace BetterSongSearch::UI {
    // Global variables
    struct DataHolder {
        inline static std::unordered_set<const SDC_wrapper::BeatStarSong*> songList;
        inline static std::vector<const SDC_wrapper::BeatStarSong*> filteredSongList;
        inline static std::vector<const SDC_wrapper::BeatStarSong*> tempSongList;
        inline static std::vector<const SDC_wrapper::BeatStarSong*> searchedSongList;
        inline static std::vector<std::string> songsWithScores;
        inline static bool loadedSDC = false;
        inline static FilterOptions filterOptions;
    };

#define PROP_GET(jsonName, varName)                                \
    static auto jsonNameHash_##varName = stringViewHash(jsonName); \
    if (nameHash == (jsonNameHash_##varName))                      \
        return &varName;


    constexpr std::string_view ShortMapDiffNames(std::string_view input) {
        // order variables from most likely to least likely to at least improve branch checks
        // optimization switch idea stolen from Red's SDC wrapper. Good job red 👏
        switch (input.front()) {
            case 'E':
                // ExpertPlus
                if (input.back() == 's') {
                    return "Ex+";
                }
                // Easy
                if (input.size() == 4) {
                    return "E";
                }
                // Expert
                return "Ex";
            case 'N':
                return "N";
            case 'H':
                return "H";
            default:
                return "UNKNOWN";
        }
    }

#undef PROP_GET

}

#ifdef HotReload
DECLARE_CLASS_CODEGEN_INTERFACES(BetterSongSearch::UI::ViewControllers, SongListController, BSML::HotReloadViewController, classof(HMUI::TableView::IDataSource*),
#else
DECLARE_CLASS_CODEGEN_INTERFACES(BetterSongSearch::UI::ViewControllers, SongListController, HMUI::ViewController, classof(HMUI::TableView::IDataSource*),
#endif

    

    DECLARE_CTOR(ctor);
    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);
    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, songList);
    
    DECLARE_INSTANCE_METHOD(void, SelectSong, HMUI::TableView* table, int id);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::VerticalLayoutGroup*, scrollBarContainer);
    DECLARE_INSTANCE_FIELD(float, cellSize);


    DECLARE_OVERRIDE_METHOD_MATCH(HMUI::TableCell*, CellForIdx, &HMUI::TableView::IDataSource::CellForIdx, HMUI::TableView* tableView, int idx);
    DECLARE_OVERRIDE_METHOD_MATCH(float, CellSize, &HMUI::TableView::IDataSource::CellSize);
    DECLARE_OVERRIDE_METHOD_MATCH(int, NumberOfCells, &HMUI::TableView::IDataSource::NumberOfCells);
    
    DECLARE_INSTANCE_METHOD(void, SelectRandom);
    DECLARE_INSTANCE_METHOD(void, ShowMoreModal);
    DECLARE_INSTANCE_METHOD(void, HideMoreModal);
    DECLARE_INSTANCE_METHOD(void, UpdateDataAndFilters);

    // Forced close modal methods
    DECLARE_INSTANCE_METHOD(void, ForcedUIClose);
    DECLARE_INSTANCE_METHOD(void, ForcedUICloseCancel);

    DECLARE_INSTANCE_METHOD(void, ShowPlaylistCreation);
    DECLARE_INSTANCE_METHOD(void, ShowSettings);

    // Play/DL
    DECLARE_INSTANCE_METHOD(void, Download);
    DECLARE_INSTANCE_METHOD(void, Play);
    DECLARE_INSTANCE_METHOD(void, ShowSongDetails);
    DECLARE_INSTANCE_METHOD(void, FilterByUploader);

    
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalOrVerticalLayoutGroup*, searchBoxContainer);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalOrVerticalLayoutGroup*, detailActions);
    DECLARE_INSTANCE_FIELD(HMUI::ModalView*, rootModal);
    DECLARE_INSTANCE_FIELD(HMUI::ModalView*, moreModal);
    DECLARE_INSTANCE_FIELD(HMUI::ModalView*, downloadCancelConfirmModal);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongKey);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedRating);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedCharacteristics);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, songDetailsLoading);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, searchInProgress);

    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongAuthor);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongName);

    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, coverImage);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, coverLoading);

    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongDiffInfo);

    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, downloadButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, playButton);

    BSML_OPTIONS_LIST_OBJECT(sortModeSelections, "Newest", "Oldest", "Latest Ranked", "Most Stars", "Least Stars", "Best rated", "Worst rated");

    DECLARE_INSTANCE_FIELD(BSML::DropdownListSetting*, sortDropdown);
    DECLARE_INSTANCE_FIELD(StringW, selectedSortMode);



public:
    HMUI::TableView* songListTable() {
        if(songList) {return songList->tableView;} else return nullptr;
    }
        
    std::vector<std::string> songsWithScores;
    FilterOptions filterOptions;

    bool DifficultyCheck(const SDC_wrapper::BeatStarSongDifficultyStats* diff, const SDC_wrapper::BeatStarSong* song);
    bool MeetsFilter(const SDC_wrapper::BeatStarSong* song);

    void SetSelectedSong(const SDC_wrapper::BeatStarSong*);


    BetterSongSearch::Util::RatelimitCoroutine* limitedUpdateSearchedSongsList = nullptr;

    void SortAndFilterSongs(SortMode sort, std::string_view search, bool resetTable);
    void ResetTable();
)