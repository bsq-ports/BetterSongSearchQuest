#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "HMUI/TableView_IDataSource.hpp"
#include "HMUI/TableView.hpp"
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
#include "main.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#ifndef DECLARE_OVERRIDE_METHOD_MATCH
#define DECLARE_OVERRIDE_METHOD_MATCH(retval, method, mptr, ...) \
    DECLARE_OVERRIDE_METHOD(retval, method, il2cpp_utils::il2cpp_type_check::MetadataGetter<mptr>::get(), __VA_ARGS__)
#endif


#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

inline static const int RETRY_COUNT = 3;

class DownloadHistoryEntry {
public:
    enum DownloadStatus {
        Downloading,
        Preparing,
        Extracting,
        Queued,
        Failed,
        Downloaded,
        Loaded
    };
    DownloadStatus status = DownloadStatus::Queued;
    std::string statusDetails = "";
    std::string statusMessage() {return fmt::format("{} {}", status, statusDetails);}
    float downloadProgress = 1.0f;
    int retries = 0;
    bool isDownloading() { return status == DownloadStatus::Downloading || status == DownloadStatus::Preparing || status == DownloadStatus::Extracting;}
    bool isQueued() { return status == DownloadStatus::Queued || (status == DownloadStatus::Failed && retries < RETRY_COUNT);}
    std::string songName;
    std::string levelAuthorName;
    std::string key;
    std::string hash;

    int orderValue() { return ((int)status * 100) + retries; }

    bool IsInAnyOfStates(DownloadStatus states) {
        return (status & states) != 0;
    }

    void ResetIfFailed() {
        if(status != DownloadStatus::Failed || retries < RETRY_COUNT)
            return;

        status = DownloadStatus::Queued;
        retries = 0;
    }

    DownloadHistoryEntry(const SDC_wrapper::BeatStarSong* song) {
        songName = song->song_name.string_data;
        levelAuthorName = song->level_author_name.string_data;
        key = song->key.string_data;
        hash = song->hash.string_data;
    }

    std::function<void()> UpdateProgressHandler;
};
___DECLARE_TYPE_WRAPPER_INHERITANCE(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController, Il2CppTypeEnum::IL2CPP_TYPE_CLASS, HMUI::ViewController, "BetterSongSearch::UI::ViewControllers", {classof(HMUI::TableView::IDataSource*)}, 0, nullptr,
    DECLARE_CTOR(ctor);
    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);
    DECLARE_INSTANCE_FIELD(BSML::CustomListTableData*, downloadList);
    DECLARE_INSTANCE_METHOD(void, SelectSong, HMUI::TableView* table, int id);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::VerticalLayoutGroup*, scrollBarContainer);
    DECLARE_INSTANCE_FIELD(float, cellSize);
    DECLARE_OVERRIDE_METHOD_MATCH(HMUI::TableCell*, CellForIdx, &HMUI::TableView::IDataSource::CellForIdx, HMUI::TableView* tableView, int idx);
    DECLARE_OVERRIDE_METHOD_MATCH(float, CellSize, &HMUI::TableView::IDataSource::CellSize);
    DECLARE_OVERRIDE_METHOD_MATCH(int, NumberOfCells, &HMUI::TableView::IDataSource::NumberOfCells);
    

    HMUI::TableView* downloadHistoryTable() {if(downloadList) return downloadList->tableView;}
public:
    std::vector<DownloadHistoryEntry*> downloadEntryList;
    bool TryAddDownload(const SDC_wrapper::BeatStarSong* song);
    bool hasUnloadedDownloads() {
        bool x = false;
        for(auto entry : downloadEntryList) {
            if(entry->status == DownloadHistoryEntry::DownloadStatus::Downloaded) {
                x = true;
                break;
            } else
                continue;
        }
        return x;
    }
    const int MAX_PARALLEL_DOWNLOADS = 2;
    
    
)