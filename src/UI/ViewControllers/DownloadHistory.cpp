#include "UI/ViewControllers/DownloadHistory.hpp"

#include <mutex>
#include <shared_mutex>

#include "assets.hpp"
#include "beatsaverplusplus/shared/BeatSaver.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "HMUI/TableView.hpp"
#include "logging.hpp"
#include "songcore/shared/SongCore.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "UnityEngine/Resources.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "Util/TextUtil.hpp"
#include "web-utils/shared/WebUtils.hpp"

using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController);

void errored(std::string message, DownloadHistoryEntry* entry) {
    std::unique_lock<std::shared_mutex> lock(entry->syncMutex);
    entry->status = DownloadStatus::Failed;
    entry->statusDetails = fmt::format(": {}", message);
    entry->retries = 69;
}

struct CustomDLBeatMapResponse {
    std::optional<std::filesystem::path> path;
    int curlStatus;
    int responseCode;
};

CustomDLBeatMapResponse CustomDownloadBeatmap(BeatSaver::API::BeatmapDownloadInfo info, std::function<void(float)> progressReport = nullptr) {
    auto [options, response] = DownloadBeatmapURLOptionsAndResponse(info);
    BeatSaver::API::GetBeatsaverDownloader().GetInto(options, &response, progressReport);
    return {response.responseData, response.get_CurlStatus(), response.get_HttpCode()};
}

void ViewControllers::DownloadHistoryViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) {
        return;
    }

    limitedFullTableReload = new BetterSongSearch::Util::RatelimitCoroutine(
        [this]() {
            BSML::MainThreadScheduler::Schedule([this] {
                auto table = this->downloadHistoryTable();
                if (table) {
                    table->ReloadData();
                }
            });
        },
        0.2f
    );

    INFO("Download contoller activated");
    BSML::parse_and_construct(Assets::DownloadHistory_bsml, this->get_transform(), this);

    if (this->downloadList != nullptr) {
        INFO("Table exists");
        downloadList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource*>(this), false);
    }

    // BSML has a bug that stops getting the correct platform helper and on game reset it dies and the scrollhelper stays invalid and scroll doesn't
    // work
    auto platformHelper = BSML::Helpers::GetDiContainer()->Resolve<GlobalNamespace::IVRPlatformHelper*>();
    if (platformHelper == nullptr) {
    } else {
        for (auto x : this->GetComponentsInChildren<HMUI::ScrollView*>()) {
            x->____platformHelper = platformHelper;
        }
    }

#ifdef HotReload
    fileWatcher->filePath = "/sdcard/bsml/BetterSongSearch/DownloadHistory.bsml";
    fileWatcher->checkInterval = 0.5f;
#endif
}

void ViewControllers::DownloadHistoryViewController::SelectSong(HMUI::TableView* table, int id) {
    if (!fcInstance || !fcInstance->SongListController) {
        return;
    }

    std::shared_lock<std::shared_mutex> lock(downloadListMutex);
    auto length = downloadEntryList.size();
    if (id >= length) {
        WARNING("Non existent song id, id: {}, length: {}", id, length);
        return;
    }
    auto entry = downloadEntryList[id];
    lock.unlock();
    INFO("Selecting a song {}", entry->songName);

    // If downloaded then select the song
    auto entryStatus = entry->getStatus();
    if (entryStatus == DownloadStatus::Downloaded) {
        DEBUG("DOWNLOADED");
        auto controller = fcInstance->SongListController;
        controller->SelectSongByHash(entry->hash);
        controller->songListTable()->ClearSelection();
    } else if (entryStatus == DownloadStatus::Failed) {
        std::unique_lock<std::shared_mutex> lock(entry->syncMutex);
        entry->retries = 0;
        lock.unlock();
        ProcessDownloads(true);
    }

    // Reset selection
    this->downloadHistoryTable()->ClearSelection();
}

float ViewControllers::DownloadHistoryViewController::CellSize() {
    return this->cellSize;
}

int ViewControllers::DownloadHistoryViewController::NumberOfCells() {
    std::shared_lock<std::shared_mutex> lock(downloadListMutex);
    return downloadEntryList.size();
}

void ViewControllers::DownloadHistoryViewController::ctor() {
    INVOKE_CTOR();
    this->cellSize = 8.05f;
}

// BSML::CustomCellInfo
HMUI::TableCell* ViewControllers::DownloadHistoryViewController::CellForIdx(HMUI::TableView* tableView, int idx) {
    std::shared_lock<std::shared_mutex> lock(downloadListMutex);
    auto entry = downloadEntryList[idx];
    lock.unlock();

    return ViewControllers::DownloadListTableData::GetCell(tableView)->PopulateWithSongData(entry);
}

bool ViewControllers::DownloadHistoryViewController::TryAddDownload(SongDetailsCache::Song const* song, bool isBatch) {
    DownloadHistoryEntry* existingDLHistoryEntry = nullptr;

    {
        std::shared_lock<std::shared_mutex> lock(downloadListMutex);
        for (auto entry : downloadEntryList) {
            if (entry->key == song->key()) {
                existingDLHistoryEntry = entry;
                break;
            }
        }
    }

    if (existingDLHistoryEntry) {
        existingDLHistoryEntry->ResetIfFailed();
    }

    if (!CheckIsDownloadable(song->hash())) {
        return false;
    }

    if (existingDLHistoryEntry == nullptr) {
        std::unique_lock<std::shared_mutex> lock(downloadListMutex);
        downloadEntryList.push_back(new DownloadHistoryEntry(song));
        lock.unlock();

        downloadHistoryTable()->ReloadData();
        downloadHistoryTable()->ScrollToCellWithIdx(0, HMUI::TableView::ScrollPositionType::Beginning, false);
    } else {
        std::unique_lock<std::shared_mutex> lock(existingDLHistoryEntry->syncMutex);
        existingDLHistoryEntry->status = DownloadStatus::Queued;
    }

    ProcessDownloads(!isBatch);

    return true;
}

void ViewControllers::DownloadHistoryViewController::ProcessDownloads(bool forceTableReload) {
    if (!this->get_gameObject()->get_activeInHierarchy()) {
        return;
    }

    int count = 0;  // Count the ones  that need to be downloaded
    {
        std::shared_lock<std::shared_mutex> lock(downloadListMutex);
        for (auto entry : downloadEntryList) {
            if (entry->IsInAnyOfStates((DownloadStatus) (DownloadStatus::Preparing | DownloadStatus::Downloading))) {
                count++;
            }
        }
    }

    if (count >= MAX_PARALLEL_DOWNLOADS) {
        if (forceTableReload) {
            this->RefreshTable();
        }
        return;
    }

    // Get first entry
    DownloadHistoryEntry* currentEntry = nullptr;
    {
        std::shared_lock<std::shared_mutex> downloadEntryListLock(downloadListMutex);
        for (auto entry : downloadEntryList) {
            if (entry->getRetries() < RETRY_COUNT && entry->IsInAnyOfStates((DownloadStatus) (DownloadStatus::Failed | DownloadStatus::Queued))) {
                if (!currentEntry) {
                    currentEntry = entry;
                    continue;
                }
                if (entry->orderValue() > currentEntry->orderValue()) {
                    currentEntry = entry;
                }
            }
        }
    }

    if (!currentEntry) {
        RefreshTable(forceTableReload);
        return;
    }

    // We have the entry, now we need to download
    if (currentEntry->getStatus() == DownloadStatus::Failed) {
        std::unique_lock<std::shared_mutex> lock(currentEntry->syncMutex);
        currentEntry->retries++;
    }

    {
        std::unique_lock<std::shared_mutex> lock(currentEntry->syncMutex);
        currentEntry->downloadProgress = 0.0f;
        currentEntry->status = DownloadStatus::Downloading;
        currentEntry->lastUpdate = CurrentTimeMs();
    }

    std::function<void(float)> progressUpdate = [this, currentEntry](float downloadProgress) {
        std::shared_lock<std::shared_mutex> lock(currentEntry->syncMutex);
        auto now = CurrentTimeMs();
        if (now - currentEntry->lastUpdate < 50) {
            return;
        }
        lock.unlock();

        {
            std::unique_lock<std::shared_mutex> uniqueLock(currentEntry->syncMutex);
            currentEntry->statusDetails = fmt::format(
                "({}%{})",
                (int) round(downloadProgress * 100),
                currentEntry->retries == 0 ? "" : fmt::format(", retry {} / {}", currentEntry->retries, RETRY_COUNT)
            );
            currentEntry->lastUpdate = now;
            currentEntry->downloadProgress = downloadProgress;
            uniqueLock.unlock();
        }

        BSML::MainThreadScheduler::Schedule([currentEntry] {
            std::shared_lock<std::shared_mutex> lock(currentEntry->syncMutex);
            auto handler = currentEntry->UpdateProgressHandler;
            lock.unlock();
            if (handler == nullptr) {
                return;
            }
            handler();
        });

        DEBUG("DownloadProgress: {}", downloadProgress);
    };

    RefreshTable(true);

    std::thread([this, currentEntry, forceTableReload, progressUpdate] {
        auto response = WebUtils::Get<BeatSaver::API::BeatmapResponse>(BeatSaver::API::GetBeatmapByHashURLOptions(std::string(currentEntry->hash)));

        if (!response.IsSuccessful()) {
            int responseCode = response.httpCode;
            int curlStatus = response.curlStatus;

            BSML::MainThreadScheduler::Schedule([this, currentEntry, forceTableReload, responseCode, curlStatus] {
                std::string message = "";

                if (curlStatus != 0) {
                    message = Util::curlErrorToString(curlStatus);
                    message = fmt::format("Curl: {}", message);
                } else {
                    if (responseCode == 404) {
                        message = "Song is deleted";
                    } else {
                        message = Util::httpErrorToString(responseCode);
                    }
                }

                errored(message, currentEntry);
                RefreshTable(true);
                this->ProcessDownloads(forceTableReload);
            });
            return;
        }

        auto gotBeatmap = response.responseData.has_value();
        if (!gotBeatmap) {
            BSML::MainThreadScheduler::Schedule([this, currentEntry, forceTableReload] {
                errored("Response is empty", currentEntry);
                RefreshTable(true);
                this->ProcessDownloads(forceTableReload);
            });
            return;
        }

        auto beatmap = response.responseData.value();
        DEBUG("Beatmap name: {}", beatmap.GetName());

        auto dlInfo = CustomDownloadBeatmap(BeatSaver::API::BeatmapDownloadInfo(beatmap), progressUpdate);

        BSML::MainThreadScheduler::Schedule([dlInfo, this, currentEntry, forceTableReload] {
            auto path = dlInfo.path;
            int curlStatus = dlInfo.curlStatus;
            int responseCode = dlInfo.responseCode;

            if (!path.has_value()) {
                DEBUG("ERROR DOWNLOADING SONG");

                std::string message = "";
                if (curlStatus != 0) {
                    message = Util::curlErrorToString(curlStatus);
                    message = fmt::format("Curl: {}", message);
                } else {
                    if (responseCode == 404) {
                        message = "Song file not found";
                    } else {
                        message = Util::httpErrorToString(responseCode);
                    }
                }

                DEBUG("Error downloading the song: {}", message);
                errored(message, currentEntry);
                RefreshTable(true);
                this->ProcessDownloads(forceTableReload);
            } else {
                {
                    std::unique_lock<std::shared_mutex> lock(currentEntry->syncMutex);
                    currentEntry->status = DownloadStatus::Downloaded;
                    currentEntry->statusDetails = "";
                    currentEntry->downloadProgress = 1.0f;
                }
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

            if (fcInstance && fcInstance->SongListController) {
                auto currentSong = fcInstance->SongListController->GetCurrentSong();
                if (currentSong != nullptr) {
                    auto entryStatus = currentEntry->getStatus();
                    if (entryStatus == DownloadStatus::Downloaded) {
                        // NESTING HELLLL
                        if (currentSong->hash() == currentEntry->hash) {
                            fcInstance->SongListController->SetIsDownloaded(true);
                        }
                        fcInstance->SongListController->songListTable()->RefreshCells(false, true);
                    } else {
                        if (currentSong->hash() == currentEntry->hash) {
                            fcInstance->SongListController->SetIsDownloaded(false);
                        }
                    }
                }
            }
        });
    }).detach();

    this->ProcessDownloads(forceTableReload);
}

void ViewControllers::DownloadHistoryViewController::RefreshTable(bool fullReload) {
    BSML::MainThreadScheduler::Schedule([this] {
        DEBUG("Refreshing table");
        // Sort entry list
        {
            std::unique_lock<std::shared_mutex> lock(downloadListMutex);
            std::stable_sort(downloadEntryList.begin(), downloadEntryList.end(), [](DownloadHistoryEntry* entry1, DownloadHistoryEntry* entry2) {
                return (entry1->orderValue() < entry2->orderValue());
            });
        }
        DEBUG("Starting coroutine to refresh table");
        this->StartCoroutine(custom_types::Helpers::new_coro(this->limitedFullTableReload->Call()));
    });
}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloadedAndLoaded(std::string songHash) {
    auto level = SongCore::API::Loading::GetLevelByHash(songHash);
    return level != nullptr;
};

DownloadHistoryEntry* ViewControllers::DownloadHistoryViewController::GetDownloadByHash(std::string hash) {
    std::shared_lock<std::shared_mutex> lock(downloadListMutex);
    for (auto entry : this->downloadEntryList) {
        if (entry->hash == hash) {
            return entry;
        }
    }
    return nullptr;
}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloadable(DownloadHistoryEntry* entry) {
    auto dlElem = entry;
    if (dlElem == nullptr) {
        return true;
    }
    if (dlElem->getRetries() == 3 && dlElem->getStatus() == DownloadStatus::Failed) {
        return true;
    }

    if (!dlElem->IsInAnyOfStates((DownloadStatus) (DownloadStatus::Preparing | DownloadStatus::Downloading | DownloadStatus::Queued)) &&
        !CheckIsDownloaded(dlElem->hash)) {
        return true;
    }

    return false;
}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloaded(std::string songHash) {
    auto entry = this->GetDownloadByHash(songHash);
    bool downloadedInList = false;

    if (entry != nullptr && entry->getStatus() == DownloadStatus::Downloaded) {
        downloadedInList = true;
    };
    return (downloadedInList || CheckIsDownloadedAndLoaded(songHash));
}

bool ViewControllers::DownloadHistoryViewController::CheckIsDownloadable(std::string songHash) {
    auto dlElem = this->GetDownloadByHash(songHash);
    return this->CheckIsDownloadable(dlElem);
}

bool ViewControllers::DownloadHistoryViewController::HasPendingDownloads() {
    std::shared_lock<std::shared_mutex> lock(downloadListMutex);
    for (auto entry : downloadEntryList) {
        if (entry->IsInAnyOfStates((DownloadStatus) (DownloadStatus::Downloading | DownloadStatus::Queued))) {
            return true;
        }
    }
    return false;
};
