#include "UI/ViewControllers/DownloadHistoryCell.hpp"

#include <mutex>
#include <shared_mutex>

#include "UnityEngine/RectTransform.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomDownloadListTableCell)

namespace BetterSongSearch::UI::ViewControllers {
    CustomDownloadListTableCell* CustomDownloadListTableCell::PopulateWithSongData(DownloadHistoryEntry* entry) {
        songName->set_text(entry->songName);
        levelAuthorName->set_text(entry->levelAuthorName);
        this->entry = entry;
        statusLabel->set_text(entry->statusMessage());

        // Cells persist so it seems to be safe to just store this. If the entry changes we will just update the cell with the new data
        std::unique_lock<std::shared_mutex> lock(entry->syncMutex);
        entry->UpdateProgressHandler = [this]() {
            UpdateProgress();
        };
        return this;
    }

    void CustomDownloadListTableCell::RefreshBgState() {
        bgContainer->set_color(UnityEngine::Color(0, 0, 0, highlighted ? 0.8f : 0.45f));
        RefreshBar();
    }

    void CustomDownloadListTableCell::RefreshBar() {
        if (!entry) {
            return;
        }
        std::shared_lock<std::shared_mutex> lock(entry->syncMutex);
        auto status = entry->status;
        auto clr = status == DownloadStatus::Failed                 ? UnityEngine::Color::get_red()
                 : status != DownloadStatus::Queued ? UnityEngine::Color::get_green()
                                                                                 : UnityEngine::Color::get_gray();
        clr.a = 0.5f + (entry->downloadProgress * 0.4f);
        bgProgress->set_color(clr);

        auto x = bgProgress->get_gameObject()->get_transform().cast<UnityEngine::RectTransform>();
        if (!x) {
            return;
        }
        x->set_anchorMax(UnityEngine::Vector2(entry->downloadProgress, 1));
        x->ForceUpdateRectTransforms();
    }

    void CustomDownloadListTableCell::UpdateProgress() {
        if (!entry) return;
        statusLabel->set_text(entry->statusMessage());
        RefreshBar();
    }

    void CustomDownloadListTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        RefreshBgState();
    }

    void CustomDownloadListTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        RefreshBgState();
    }

    void CustomDownloadListTableCell::WasPreparedForReuse() {
        if (!entry) {
            return;
        }
        std::unique_lock<std::shared_mutex> lock(entry->syncMutex);
        entry->UpdateProgressHandler = nullptr;
        entry = nullptr;
    }
}  // namespace BetterSongSearch::UI::ViewControllers
