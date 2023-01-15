#pragma once
#include "UI/ViewControllers/SongListCellTableData.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/TableView_IDataSource.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableCell.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/TextPageScrollView.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/SegmentedControl/CustomTextSegmentedControlData.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/MultiplayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator_State.hpp"
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/ViewControllers/HotReloadViewController.hpp"
#include "bsml/shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"
#include "songloader/shared/API.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"
#include "song-details/shared/Data/Song.hpp"
#include "song-details/shared/Data/SongDifficulty.hpp"
#include "song-details/shared/Data/MapCharacteristic.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/UI/HorizontalOrVerticalLayoutGroup.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/CancellationTokenSource.hpp"
#include "main.hpp"
#include "FilterOptions.hpp"
#include "Util/RatelimitCoroutine.hpp"
#include "UI/Modals/MultiDL.hpp"
#include "UI/Modals/Settings.hpp"
#include "UI/Modals/UploadDetails.hpp"
#include "song-details/shared/SongDetails.hpp"
#include "song-details/shared/Data/Song.hpp"
#include <fmt/chrono.h>

#ifndef DECLARE_OVERRIDE_METHOD_MATCH
#define DECLARE_OVERRIDE_METHOD_MATCH(retval, method, mptr, ...) \
    DECLARE_OVERRIDE_METHOD(retval, method, il2cpp_utils::il2cpp_type_check::MetadataGetter<mptr>::get(), __VA_ARGS__)
#endif


#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

namespace BetterSongSearch::UI {
    inline GlobalNamespace::IPreviewBeatmapLevel* currentLevel;
    inline bool fromBSS = false;
    inline bool openToCustom = false;
    bool MeetsFilter(const SongDetailsCache::Song* song);
    bool DifficultyCheck(const SongDetailsCache::SongDifficulty* diff, const SongDetailsCache::Song* song);

    // Global variables
    struct DataHolder {
        // Song details ref
        inline static SongDetailsCache::SongDetails* songDetails;
        // Filtered songs
        inline static std::vector<const SongDetailsCache::Song*> filteredSongList;
        // Searched songs
        inline static std::vector<const SongDetailsCache::Song*> searchedSongList;
        // Sorted songs (actually displayed)
        inline static std::vector<const SongDetailsCache::Song*> sortedSongList;
        
        inline static std::vector<std::string> songsWithScores;

        inline static FilterOptions filterOptions;
        inline static FilterOptionsCache filterOptionsCache;
        /// @brief Player data model to get the scores
        inline static GlobalNamespace::PlayerDataModel* playerDataModel = nullptr;


        /// @brief Song data is loaded
        inline static bool loaded = false;
        /// @brief Song data failed to load
        inline static bool failed = false;
        // Song data is loading 
        inline static bool loading = false;
        /// @brief Flag to say that the song list needs a refresh when the user opens the BSS because the data got updated
        inline static bool needsRefresh = false;
        // Song list is invalid (means that we should not touch anything in the song list rn)
        inline static bool invalid = false;

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
    DECLARE_DTOR(dtor);
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


    DECLARE_INSTANCE_METHOD(void, ShowBatchDownload);
    DECLARE_INSTANCE_METHOD(void, ShowPlaylistCreation);
    DECLARE_INSTANCE_METHOD(void, ShowSettings);
    DECLARE_INSTANCE_METHOD(void, ShowCloseConfirmation);
    DECLARE_INSTANCE_METHOD(void, ForcedUIClose);
    DECLARE_INSTANCE_METHOD(void, ForcedUICloseCancel);
    
    

    // Play/DL
    DECLARE_INSTANCE_METHOD(void, Download);
    DECLARE_INSTANCE_METHOD(void, PostParse);
    DECLARE_INSTANCE_METHOD(void, Play);
    
    DECLARE_INSTANCE_METHOD(void, ShowSongDetails);
    DECLARE_INSTANCE_METHOD(void, FilterByUploader);
    DECLARE_INSTANCE_METHOD(void, UpdateSearchedSongsList);
    DECLARE_INSTANCE_METHOD(void, _UpdateSearchedSongsList);

    DECLARE_INSTANCE_FIELD(bool, IsSearching);
    
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalOrVerticalLayoutGroup*, searchBoxContainer);
    DECLARE_INSTANCE_FIELD(HMUI::CurvedTextMeshPro*, songSearchPlaceholder);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalOrVerticalLayoutGroup*, detailActions);
    DECLARE_INSTANCE_FIELD(HMUI::ModalView*, rootModal);
    DECLARE_INSTANCE_FIELD(HMUI::ModalView*, moreModal);
    DECLARE_INSTANCE_FIELD(BSML::ModalView*, downloadCancelConfirmModal);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongKey);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedRating);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedCharacteristics);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, songDetailsLoading);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, searchInProgress);

    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongAuthor);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongName);

    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject *, searchBox);
    DECLARE_INSTANCE_FIELD(UnityEngine::Sprite*, defaultImage);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, coverImage);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, coverLoading);

    // Modals
    DECLARE_INSTANCE_FIELD(Modals::MultiDL*, multiDlModal);
    DECLARE_INSTANCE_FIELD(Modals::Settings*, settingsModal);
    DECLARE_INSTANCE_FIELD(Modals::UploadDetails*, uploadDetailsModal);
    
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, selectedSongDiffInfo);

    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, downloadButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, playButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, infoButton);
    BSML_OPTIONS_LIST_OBJECT(sortModeSelections, "Newest", "Oldest", "Latest Ranked", "Most Stars", "Least Stars", "Best rated", "Worst rated");

    DECLARE_INSTANCE_FIELD(BSML::DropdownListSetting*, sortDropdown);
    DECLARE_INSTANCE_FIELD(StringW, selectedSortMode);

    DECLARE_INSTANCE_FIELD(GlobalNamespace::SoloFreePlayFlowCoordinator*, soloFreePlayFlowCoordinator);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::MultiplayerLevelSelectionFlowCoordinator*, multiplayerLevelSelectionFlowCoordinator);


    DECLARE_INSTANCE_FIELD(System::Threading::CancellationTokenSource*, songAssetLoadCanceller);

public:
    HMUI::TableView* songListTable() {
        if(songList) {return songList->tableView;} else return nullptr;
    }

    void SelectSongByHash(std::string hash);
    void SetSelectedSong(const SongDetailsCache::Song* song);


    BetterSongSearch::Util::RatelimitCoroutine* limitedUpdateSearchedSongsList = nullptr;

    void SortAndFilterSongs(SortMode sort, std::string_view search, bool resetTable);
    void ResetTable();
    const SongDetailsCache::Song* currentSong = nullptr;
    void UpdateDetails();
    void SetIsDownloaded(bool isDownloaded, bool downloadable = true);

    // Temp values
    std::string search = "";
    SortMode sort = (SortMode) 0;
    // Prev values
    std::string prevSearch = "";
    SortMode prevSort = (SortMode) 0;
    
    bool filterChanged = true;

    void UpdateSearch();
    custom_types::Helpers::Coroutine UpdateDataAndFiltersCoro();


    void PlaySong(const SongDetailsCache::Song* song = nullptr);
    void DownloadSongList();
    void RetryDownloadSongList();

    void EnterSolo(GlobalNamespace::IPreviewBeatmapLevel* level);

    // Event receivers
    void SongDataDone();
    void SongDataError();
)

