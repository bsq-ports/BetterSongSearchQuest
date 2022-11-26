#include "UI/ViewControllers/DownloadHistory.hpp"
#include "main.hpp"


#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/TableView.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "bsml/shared/BSML.hpp"
#include "assets.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, DownloadHistoryViewController);

void ViewControllers::DownloadHistoryViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    getLoggerOld().info("Download contoller activated");
    BSML::parse_and_construct(IncludedAssets::DownloadHistory_bsml, this->get_transform(), this);

    if (this->downloadList != nullptr && this->downloadList->m_CachedPtr.m_value != nullptr ) { 
        getLoggerOld().info("Table exists");
        
        downloadList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource*>(this), false);
        // ViewControllers::DownloadListTableData tableData = 
        // auto tableView = descriptorListTableData->tableView;

    }
}

void  ViewControllers::DownloadHistoryViewController::SelectSong(HMUI::TableView* table, int id) {
    getLoggerOld().info("Cell clicked %i", id);
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
    // downloadEntryList = std::vector<DownloadHistoryEntry*>();
}
// BSML::CustomCellInfo
HMUI::TableCell* ViewControllers::DownloadHistoryViewController::CellForIdx(HMUI::TableView* tableView, int idx)
{
    return ViewControllers::DownloadListTableData::GetCell(tableView)->PopulateWithSongData(downloadEntryList[idx]);
}


bool ViewControllers::DownloadHistoryViewController::TryAddDownload(const SDC_wrapper::BeatStarSong* song) {
        DownloadHistoryEntry* existingDLHistoryEntry = nullptr;
        
        if (downloadEntryList.size() > 0) {
            
            for(auto entry : downloadEntryList) {
                
                if(entry->key == song->key.string_data) {
                    existingDLHistoryEntry = entry;
                    break;
                }
            }
        }
        
        

        if(existingDLHistoryEntry) existingDLHistoryEntry->ResetIfFailed();
        if(existingDLHistoryEntry == nullptr) {
            //var newPos = downloadList.FindLastIndex(x => x.status > DownloadHistoryEntry.DownloadStatus.Queued);
            downloadEntryList.push_back(new DownloadHistoryEntry(song));
            downloadHistoryTable()->ReloadData();
            downloadHistoryTable()->ScrollToCellWithIdx(0, TableView::ScrollPositionType::Beginning, false);
        } else {
            existingDLHistoryEntry->status = DownloadHistoryEntry::DownloadStatus::Queued;
        }
        DEBUG("1");
        // if(!song.CheckIsDownloadable())
        // return false;
        
        return true;
}