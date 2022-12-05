#include "UI/ViewControllers/SongListCell.hpp"
#include "UnityEngine/RectTransform.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell)


namespace BetterSongSearch::UI::ViewControllers {

    CustomSongListTableCell* CustomSongListTableCell::PopulateWithSongData(const SDC_wrapper::BeatStarSong* entry) {
        // songName->set_text(entry->songName);
        // levelAuthorName->set_text(entry->levelAuthorName);
        // statusLabel->set_text(entry->statusMessage());
        // this->entry = entry;
        return this;
    }

    void  CustomSongListTableCell::RefreshBgState() {
        // bgContainer->set_color(UnityEngine::Color(0, 0, 0, highlighted ? 0.8f : 0.45f));
    }

    void  CustomSongListTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        RefreshBgState();
    }

    void  CustomSongListTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        RefreshBgState();
    }

    void CustomSongListTableCell::WasPreparedForReuse() {
       entry = nullptr;
    }

    void  CustomSongListTableCell::SetFontSizes(){
        return;
    };
}

