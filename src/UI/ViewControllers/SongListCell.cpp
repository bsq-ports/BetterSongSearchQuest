#include "UI/ViewControllers/SongListCell.hpp"
#include "main.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Color.hpp"
#include "PluginConfig.hpp"
#include <fmt/chrono.h>
#include <fmt/core.h>
#include "sombrero/shared/FastColor.hpp"
#include "BeatSaverRegionManager.hpp"
#include "songloader/shared/API.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell)


namespace BetterSongSearch::UI::ViewControllers {

    CustomSongListTableCell* CustomSongListTableCell::PopulateWithSongData(const SDC_wrapper::BeatStarSong* entry) {
        DEBUG("INIT CELL");
        this->levelAuthorName->set_text(entry->GetAuthor());
        this->songLengthAndRating->set_text(fmt::format("Length: {:%M:%S} Upvotes: {}, Downvotes: {}", std::chrono::seconds(entry->duration_secs), entry->upvotes, entry->downvotes));
        this->uploadDateFormatted->set_text(fmt::format("{:%d. %b %Y}", fmt::localtime(entry->uploaded_unix_time)));

        auto ranked = entry->GetMaxStarValue() > 0;
        bool downloaded = RuntimeSongLoader::API::GetLevelByHash(entry->hash.string_data).has_value();

        // Sombrero::FastColor songColor = Sombrero::FastColor::white();

        // these double assignments wil be optimized out, don't worry about it!
        // if (downloaded) {
        //     songColor = Sombrero::FastColor(0.53f, 0.53f, 0.53f, 1.0f);
        // }

        // if (ranked) {
        //     songColor = UnityEngine::Color(1, 0.647f, 0, 1);
        // }

        this->fullFormattedSongName->set_text(fmt::format("{} - {}", entry->GetName(), entry->GetSongAuthor()));
        // this->fullFormattedSongName->set_color(songColor);
        
        this->entry = entry;
    
        SetFontSizes();

        return this;
    }

    void  CustomSongListTableCell::RefreshBgState() {
        bgContainer->set_color(UnityEngine::Color(0, 0, 0, highlighted ? 0.8f : 0.45f));
    }

    void  CustomSongListTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        DEBUG("Selection change");
        RefreshBgState();
    }

    void  CustomSongListTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
        DEBUG("Highlight change");
        RefreshBgState();
    }

    void CustomSongListTableCell::WasPreparedForReuse() {
       entry = nullptr;
    }

    void  CustomSongListTableCell::SetFontSizes(){
        // foreach(var d in diffs)
        // d.fontSize = PluginConfig.Instance.smallerFontSize ? 2.5f : 2.9f;

        // TODO: Different font sizes
        if(true) {
            fullFormattedSongName->set_fontSize(2.7f);
            uploadDateFormatted->set_fontSize(2.7f);
            levelAuthorName->set_fontSize(2.3f);
            songLengthAndRating->set_fontSize(2.5f);
        } else {
            fullFormattedSongName->set_fontSize(3.2f);
            uploadDateFormatted->set_fontSize(3.2f);
            levelAuthorName->set_fontSize(2.6f);
            songLengthAndRating->set_fontSize(3.0f);
        }
        
        return;
    };
}

