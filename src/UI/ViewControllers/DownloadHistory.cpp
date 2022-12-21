#include "UI/ViewControllers/DownloadHistory.hpp"
#include "main.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "bsml/shared/BSML.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"

#include "assets.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

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

    getLoggerOld().info("Download contoller activated");
    BSML::parse_and_construct(IncludedAssets::DownloadHistory_bsml, this->get_transform(), this);

    if (this->downloadList != nullptr && this->downloadList->m_CachedPtr.m_value != nullptr)
    {
        getLoggerOld().info("Table exists");

        downloadList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);


    }

    limitedFullTableReload = new BetterSongSearch::Util::RatelimitCoroutine([this](){
        this->downloadHistoryTable()->ReloadData();
    }, 0.1f);



    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/DownloadHistory.bsml";
    #endif
}

void ViewControllers::DownloadHistoryViewController::SelectSong(HMUI::TableView *table, int id)
{
    DEBUG("Cell is clicked");
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

bool ViewControllers::DownloadHistoryViewController::TryAddDownload(const SDC_wrapper::BeatStarSong *song, bool isBatch)
{
    DownloadHistoryEntry *existingDLHistoryEntry = nullptr;

    for (auto entry : downloadEntryList)
    {

        if (entry->key == song->key.string_data)
        {
            existingDLHistoryEntry = entry;
            break;
        }
    }

    if (existingDLHistoryEntry)
        existingDLHistoryEntry->ResetIfFailed();
    if (existingDLHistoryEntry == nullptr)
    {
        // var newPos = downloadList.FindLastIndex(x => x.status > DownloadHistoryEntry.DownloadStatus.Queued);
        downloadEntryList.push_back(new DownloadHistoryEntry(song));
        downloadHistoryTable()->ReloadData();
        downloadHistoryTable()->ScrollToCellWithIdx(0, TableView::ScrollPositionType::Beginning, false);
    }
    else
    {
        existingDLHistoryEntry->status = DownloadHistoryEntry::DownloadStatus::Queued;
    }

    ProcessDownloads(!isBatch);
    // TODO: Add downloadable check
    // if(!song.CheckIsDownloadable())
    // return false;

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
    
    std::function<void(float)> progressUpdate = [this,  firstEntry](float downloadPercentage)
    {

        auto now = CurrentTimeMs();
        if (now - firstEntry->lastUpdate < 50) {
            return;
        }
        
        firstEntry->statusDetails = fmt::format("({}{})", downloadPercentage, firstEntry->retries == 0 ? "": fmt::format(", retry {} / {}", firstEntry->retries, RETRY_COUNT));
        firstEntry->lastUpdate = now;

        firstEntry->downloadProgress = downloadPercentage / 100.0f;
        if(firstEntry->UpdateProgressHandler != nullptr) {
            QuestUI::MainThreadScheduler::Schedule([firstEntry]{
                firstEntry->UpdateProgressHandler();
            });
        }
        fmtLog(Logging::Level::INFO, "DownloadProgress: {0:.2f}", downloadPercentage);     
    };
    DEBUG("Hash {}", firstEntry->hash);

    firstEntry->downloadProgress = 0.0f;
    firstEntry->status = DownloadHistoryEntry::DownloadStatus::Downloading;

    RefreshTable(true);

    BeatSaver::API::GetBeatmapByHashAsync(std::string(firstEntry->hash),
        [this, progressUpdate, firstEntry, forceTableReload](std::optional<BeatSaver::Beatmap> beatmap)
        {
            if (beatmap.has_value())
            {
                DEBUG("Beatmap name: {}", beatmap->GetName());
                auto value = beatmap.value();
                
                
                DEBUG("1 {}", value.GetName());
                BeatSaver::API::DownloadBeatmapAsync(
                    beatmap.value(),
                    [this,firstEntry, forceTableReload](bool error)
                    {

                        QuestUI::MainThreadScheduler::Schedule(
                            [error, this, firstEntry, forceTableReload]
                            {
                                if (error) {
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
                                    RuntimeSongLoader::API::RefreshSongs(false);
                                    this->ProcessDownloads(forceTableReload);
                                }
                                if (fcInstance->SongListController->currentSong != nullptr) {
                                    if(firstEntry->status == DownloadHistoryEntry::DownloadStatus::Downloaded) {
                                        // NESTING HELLLL      
                                        if (fcInstance->SongListController->currentSong->GetHash() == firstEntry->hash) {
                                            fcInstance->SongListController->SetIsDownloaded(true);
                                        }
                                        fcInstance->SongListController->songListTable()->RefreshCells(false, true);
                                    } else {
                                        if (fcInstance->SongListController->currentSong->GetHash() == firstEntry->hash) {
                                            fcInstance->SongListController->SetIsDownloaded(false);
                                        }
                                    }
                                }
                            });
                    },
                    progressUpdate);
            }else {
            }
        });
    this->ProcessDownloads(forceTableReload);
}

void ViewControllers::DownloadHistoryViewController::RefreshTable(bool fullReload)
{
    QuestUI::MainThreadScheduler::Schedule(
    [this]
    {
        // Sort entry list
        std::stable_sort(downloadEntryList.begin(), downloadEntryList.end(),
            [] (DownloadHistoryEntry* entry1, DownloadHistoryEntry* entry2)
            {
                return (entry1->orderValue() < entry2->orderValue());
            }
        );
        coro(this->limitedFullTableReload->Call());
    });
}
