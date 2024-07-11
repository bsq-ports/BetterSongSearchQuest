#include "UI/ViewControllers/DownloadHistory.hpp"

#include <bsml/shared/BSML/MainThreadScheduler.hpp>

#include "main.hpp"
#include "logging.hpp"

#include "HMUI/TableView.hpp"

#include "bsml/shared/BSML.hpp"
#include "songcore/shared/SongCore.hpp"
#include "beatsaverplusplus/shared/BeatSaver.hpp"
#include "web-utils/shared/WebUtils.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "assets.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

#include "UnityEngine/Resources.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"

using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController);

void errored(std::string message, SafePtr<DownloadHistoryEntry> entry)
{
    entry->status = DownloadHistoryEntry::DownloadStatus::Failed;
    entry->statusDetails = fmt::format(": {}", message);
    entry->retries = 69;
}

void ViewControllers::DownloadHistoryViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    limitedFullTableReload = new BetterSongSearch::Util::RatelimitCoroutine([this](){
        BSML::MainThreadScheduler::Schedule([this]{
            auto table = this->downloadHistoryTable();
            if (table) table->ReloadData();
        });
    }, 0.2f);


    INFO("Download contoller activated");
    BSML::parse_and_construct(Assets::DownloadHistory_bsml, this->get_transform(), this);

    if (this->downloadList != nullptr)
    {
        INFO("Table exists");
        downloadList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);
    }

    // BSML has a bug that stops getting the correct platform helper and on game reset it dies and the scrollhelper stays invalid and scroll doesn't work
    auto platformHelper = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionTableView*>()->First()->GetComponentInChildren<HMUI::ScrollView*>()->____platformHelper;
    if (platformHelper == nullptr) {
    } else {
        for (auto x: this->GetComponentsInChildren<HMUI::ScrollView*>()){
            x->____platformHelper=platformHelper;
        }
    }


    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/DownloadHistory.bsml";
    #endif
}

void ViewControllers::DownloadHistoryViewController::SelectSong(HMUI::TableView *table, int id)
{
    DEBUG("Cell is clicked");
    if (id >= NumberOfCells()) {
        WARNING("Non existent song id");
        return;
    } 
    
    auto entry = downloadEntryList[id];
    INFO("Selecting a song {}", entry->songName );

    // If downloaded then select the song
    if (entry->status == DownloadHistoryEntry::DownloadStatus::Downloaded) {
        DEBUG("DOWNLOADED");
        auto controller = fcInstance->SongListController;
        controller->SelectSongByHash(entry->hash);
        controller->songListTable()->ClearSelection();
    } else if (entry->status == DownloadHistoryEntry::DownloadStatus::Failed){
        entry->retries = 0;
        ProcessDownloads(true);
    }

    // Reset selection 
    this->downloadHistoryTable()->ClearSelection();

}

float ViewControllers::DownloadHistoryViewController::CellSize()
{
    return this->cellSize;
}
int ViewControllers::DownloadHistoryViewController::NumberOfCells()
{
    return downloadEntryList.size();
}

void ViewControllers::DownloadHistoryViewController::ctor()
{
    INVOKE_CTOR();
    this->cellSize = 8.05f;
}
// BSML::CustomCellInfo
HMUI::TableCell *ViewControllers::DownloadHistoryViewController::CellForIdx(HMUI::TableView *tableView, int idx)
{
    return ViewControllers::DownloadListTableData::GetCell(tableView)->PopulateWithSongData(downloadEntryList[idx]);
}

bool ViewControllers::DownloadHistoryViewController::TryAddDownload(const SongDetailsCache::Song *song, bool isBatch)
{
    DownloadHistoryEntry *existingDLHistoryEntry = nullptr;

    for (auto entry : downloadEntryList)
    {

        if (entry->key == song->key())
        {
            existingDLHistoryEntry = entry;
            break;
        }
    }

    if (existingDLHistoryEntry)
        existingDLHistoryEntry->ResetIfFailed();

    if(!CheckIsDownloadable(song->hash()))
        return false;

    if (existingDLHistoryEntry == nullptr)
    {
        // var newPos = downloadList.FindLastIndex(x => x.status > DownloadHistoryEntry.DownloadStatus.Queued);
        downloadEntryList.push_back(new DownloadHistoryEntry(song));
        downloadHistoryTable()->ReloadData();
        downloadHistoryTable()->ScrollToCellWithIdx(0, HMUI::TableView::ScrollPositionType::Beginning, false);
    }
    else
    {
        existingDLHistoryEntry->status = DownloadHistoryEntry::DownloadStatus::Queued;
    }

    ProcessDownloads(!isBatch);

    return true;
}
void ViewControllers::DownloadHistoryViewController::ProcessDownloads(bool forceTableReload)
{

    if (!this->get_gameObject()->get_activeInHierarchy())
        return;

    // Count the ones  that need to be downloaded
    int count = 0;
    for (auto entry : downloadEntryList)
    {
        if (entry->IsInAnyOfStates((DownloadHistoryEntry::DownloadStatus)(DownloadHistoryEntry::DownloadStatus::Preparing | DownloadHistoryEntry::DownloadStatus::Downloading)))
        {
            count++;
        }
    }
    if (count >= MAX_PARALLEL_DOWNLOADS)
    {
        if (forceTableReload)
        {
            this->RefreshTable();
        }
        return;
    }
    // Get first entry
    DownloadHistoryEntry* firstEntry = nullptr;
    for (auto entry : downloadEntryList)
    {
        if (entry->retries < RETRY_COUNT && entry->IsInAnyOfStates((DownloadHistoryEntry::DownloadStatus)(DownloadHistoryEntry::DownloadStatus::Failed | DownloadHistoryEntry::DownloadStatus::Queued)))
        {
            if (!firstEntry)
            {
                firstEntry = entry;
                continue;
            }
            if (entry->orderValue() > firstEntry->orderValue())
            {
                firstEntry = entry;
            }
        }
    }

    

    if (!firstEntry)
    {
        RefreshTable(forceTableReload);
        return;
    }

    // We have the entry, now we need to download
    if (firstEntry->status == DownloadHistoryEntry::DownloadStatus::Failed)
        firstEntry->retries++;
    firstEntry->downloadProgress = 0.0f;
    firstEntry->status = DownloadHistoryEntry::DownloadStatus::Preparing;
    firstEntry->lastUpdate = CurrentTimeMs();
    RefreshTable(true);
    
    std::function<void(float)> progressUpdate = [this,  firstEntry](float downloadProgress)
    {
        auto now = CurrentTimeMs();
        if (now - firstEntry->lastUpdate < 50) {
            return;
        }
        
        firstEntry->statusDetails = fmt::format("({}%{})", (int)round(downloadProgress*100), firstEntry->retries == 0 ? "": fmt::format(", retry {} / {}", firstEntry->retries, RETRY_COUNT));
        firstEntry->lastUpdate = now;

        firstEntry->downloadProgress = downloadProgress;
        if(firstEntry->UpdateProgressHandler != nullptr) {
            BSML::MainThreadScheduler::Schedule([firstEntry]{
                firstEntry->UpdateProgressHandler();
            });
        }
        DEBUG("DownloadProgress: {}", downloadProgress);
    };
    DEBUG("Hash {}", firstEntry->hash);

    firstEntry->downloadProgress = 0.0f;
    firstEntry->status = DownloadHistoryEntry::DownloadStatus::Downloading;

    RefreshTable(true);

    std::thread([this, firstEntry, forceTableReload, progressUpdate]{
        auto response = WebUtils::Get<BeatSaver::API::BeatmapResponse>(
            BeatSaver::API::GetBeatmapByHashURLOptions(std::string(firstEntry->hash))
        );
        
        if (!response.IsSuccessful())
        {
            BSML::MainThreadScheduler::Schedule([this, firstEntry, forceTableReload]{
                errored("Failed" ,firstEntry);
                RefreshTable(true);
                this->ProcessDownloads(forceTableReload);
            });
            return;
        }

        auto gotBeatmap = response.responseData.has_value();
        if (!gotBeatmap)
        {
            BSML::MainThreadScheduler::Schedule([this, firstEntry, forceTableReload]{
                errored("Error" ,firstEntry);
                RefreshTable(true);
                this->ProcessDownloads(forceTableReload);
            });
            return;
        }

        auto beatmap = response.responseData.value();
        DEBUG("Beatmap name: {}", beatmap.GetName());

        auto path = BeatSaver::API::DownloadBeatmap(
            BeatSaver::API::BeatmapDownloadInfo(beatmap),
            progressUpdate
        );

        BSML::MainThreadScheduler::Schedule(
            [path, this, firstEntry, forceTableReload]
            {
                if (!path.has_value()) {
                    DEBUG("ERROR DOWNLOADING SONG");
                    errored("Error" ,firstEntry);
                    RefreshTable(true);
                    this->ProcessDownloads(forceTableReload);
                } else {
                    firstEntry->status = DownloadHistoryEntry::DownloadStatus::Downloaded;
                    firstEntry->statusDetails = "";
                    firstEntry->downloadProgress = 1.0f;
                    DEBUG("Success downloading the song");
                    RefreshTable(true);
                    hasUnloadedDownloads = true;
                    this->ProcessDownloads(forceTableReload);
                }
                // If has no more dls left, refresh songs
                if (!this->HasPendingDownloads()) {
                    // Do not refresh songs if not active anymore
                    if (this->get_isActiveAndEnabled()) {
                        SongCore::API::Loading::RefreshSongs(false);
                        hasUnloadedDownloads = false;
                    }
                }
                if (fcInstance->SongListController->currentSong != nullptr) {
                    if(firstEntry->status == DownloadHistoryEntry::DownloadStatus::Downloaded) {
                        // NESTING HELLLL      
                        if (fcInstance->SongListController->currentSong->hash() == firstEntry->hash) {
                            fcInstance->SongListController->SetIsDownloaded(true);
                        }
                        fcInstance->SongListController->songListTable()->RefreshCells(false, true);
                    } else {
                        if (fcInstance->SongListController->currentSong->hash() == firstEntry->hash) {
                            fcInstance->SongListController->SetIsDownloaded(false);
                        }
                    }
                }
        });
    }).detach();
    this->ProcessDownloads(forceTableReload);
}

void ViewControllers::DownloadHistoryViewController::RefreshTable(bool fullReload)
{
    BSML::MainThreadScheduler::Schedule(
    [this]
    {
        DEBUG("Refreshing table");
        // Sort entry list
        std::stable_sort(downloadEntryList.begin(), downloadEntryList.end(),
            [] (DownloadHistoryEntry* entry1, DownloadHistoryEntry* entry2)
            {
                return (entry1->orderValue() < entry2->orderValue());
            }
        );
        DEBUG("Starting coroutine to refresh table");
        this->StartCoroutine(custom_types::Helpers::new_coro(this->limitedFullTableReload->Call()));
    });
}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloadedAndLoaded(std::string songHash){
    auto level = SongCore::API::Loading::GetLevelByHash(songHash);
    return level != nullptr;
};

DownloadHistoryEntry* ViewControllers::DownloadHistoryViewController::GetDownloadByHash(std::string hash){
    for(auto entry : this->downloadEntryList) {
        if(entry->hash == hash) {
            return entry;
        }
    }
    return nullptr;
}
bool ViewControllers::DownloadHistoryViewController::CheckIsDownloadable(DownloadHistoryEntry* entry){
    auto dlElem = entry;
    if (dlElem == nullptr) return true;
    if (dlElem->retries == 3 && dlElem->status == DownloadHistoryEntry::DownloadStatus::Failed) return true;

    if (
        !dlElem->IsInAnyOfStates(
            (DownloadHistoryEntry::DownloadStatus) (
                DownloadHistoryEntry::DownloadStatus::Preparing |
                DownloadHistoryEntry::DownloadStatus::Downloading |
                DownloadHistoryEntry::DownloadStatus::Queued
            )
        ) && !CheckIsDownloaded(dlElem->hash)){
        return true;
    }

    return false;

}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloaded(std::string songHash){
    auto entry = this->GetDownloadByHash(songHash);
    bool downloadedInList = false;

    if (entry != nullptr && entry->status == DownloadHistoryEntry::DownloadStatus::Downloaded){
        downloadedInList = true;
    };
    return (downloadedInList || CheckIsDownloadedAndLoaded(songHash));
}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloadable(std::string songHash){
    auto dlElem = this->GetDownloadByHash(songHash);
    return this->CheckIsDownloadable(dlElem);
}

bool ViewControllers::DownloadHistoryViewController::HasPendingDownloads(){
    for (auto entry : downloadEntryList)
    {
        if (entry->IsInAnyOfStates((DownloadHistoryEntry::DownloadStatus)(DownloadHistoryEntry::DownloadStatus::Downloading | DownloadHistoryEntry::DownloadStatus::Queued)))
        {
            return true;
        }
    }
    return false;
};

