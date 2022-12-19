#include "UI/ViewControllers/SongListCell.hpp"

#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Color.hpp"
#include "sombrero/shared/FastColor.hpp"
#include "songloader/shared/API.hpp"

#include "BeatSaverRegionManager.hpp"
#include "PluginConfig.hpp"
#include "main.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell);


namespace BetterSongSearch::UI::ViewControllers {
    CustomSongListTableCell* CustomSongListTableCell::PopulateWithSongData(const SDC_wrapper::BeatStarSong* entry) {
        this->levelAuthorName->set_text(entry->GetAuthor());
        this->songLengthAndRating->set_text(fmt::format("Length: {:%M:%S} Upvotes: {}, Downvotes: {}", std::chrono::seconds(entry->duration_secs), entry->upvotes, entry->downvotes));
        this->uploadDateFormatted->set_text(fmt::format("{:%d. %b %Y}", fmt::localtime(entry->uploaded_unix_time)));

        auto ranked = entry->GetMaxStarValue() > 0;
        bool downloaded = RuntimeSongLoader::API::GetLevelByHash(entry->hash.string_data).has_value();

        Sombrero::FastColor songColor = Sombrero::FastColor::white();
        if (downloaded) {
            songColor = Sombrero::FastColor(0.53f, 0.53f, 0.53f, 1.0f);
        }
        if (ranked) {
            songColor = UnityEngine::Color(1, 0.647f, 0, 1);
        }

        this->fullFormattedSongName->set_text(fmt::format("{} - {}", entry->GetName(), entry->GetSongAuthor()));
        this->fullFormattedSongName->set_color(songColor);
        
        this->entry = entry;
    
        auto sortedDiffs = entry->GetDifficultyVector();
        int diffsLeft = entry->GetDifficultyVector().size();

        for(int i = 0; i < diffs.size(); i++) {
            bool isActive = diffsLeft != 0;

            diffs[i]->get_gameObject()->set_active(isActive);

            if(!isActive)
                continue;

            if(diffsLeft != 1 && i == diffs.size() - 1) {
                diffs[i]->set_text(fmt::format("<color=#0AD>{} More", diffsLeft));
            } else {
                diffs[i]->SetText(sortedDiffs[i]->GetName());
                diffsLeft--;
            }
        }


        SetFontSizes();
        return this;
    }

    void  CustomSongListTableCell::RefreshBgState() {
        bgContainer->set_color(UnityEngine::Color(0, 0, 0, selected || highlighted ? 0.8f : 0.45f));
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
        for(auto d : diffs){
             d->set_fontSize( true ? 2.5f : 2.9f);
        }

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

