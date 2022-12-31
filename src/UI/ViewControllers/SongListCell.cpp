#include "UI/ViewControllers/SongListCell.hpp"
#include "UI/ViewControllers/SongList.hpp"

#include "UnityEngine/Color.hpp"
#include "sombrero/shared/FastColor.hpp"
#include "songloader/shared/API.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"

#include "PluginConfig.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell);


namespace BetterSongSearch::UI::ViewControllers {
    static std::map<std::string, std::string> shortMapDiffNames = {
            { "Easy", "Easy" },
            { "Normal", "Norm" },
            { "Hard", "Hard" },
            { "Expert", "Ex" },
            { "ExpertPlus", "Ex+" }
    };

    static std::map<song_data_core::BeatStarCharacteristics, std::string> customCharNames = {
            { song_data_core::BeatStarCharacteristics::Degree90, "90" },
            { song_data_core::BeatStarCharacteristics::Degree360, "360" },
            { song_data_core::BeatStarCharacteristics::Lawless, "LL" },
            { song_data_core::BeatStarCharacteristics::Unknown, "?" },
            { song_data_core::BeatStarCharacteristics::Lightshow, "LS" }
    };

    std::string GetCombinedShortDiffName(int diffCount, const SDC_wrapper::BeatStarSongDifficultyStats* diff) {
        auto retVal = fmt::format("{}", (diffCount > 5 ? shortMapDiffNames[std::string(diff->GetName())] : diff->GetName()));

        if(diff->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Standard())
            return retVal;
        retVal += fmt::format("({})", customCharNames[diff->diff_characteristics]);

        return retVal;
    }

    CustomSongListTableCell* CustomSongListTableCell::PopulateWithSongData(const SDC_wrapper::BeatStarSong* entry) {
        this->levelAuthorName->set_text(entry->GetAuthor());
        this->songLengthAndRating->set_text(fmt::format("Length: {:%M:%S} Upvotes: {}, Downvotes: {}", std::chrono::seconds(entry->duration_secs), entry->upvotes, entry->downvotes));
        this->uploadDateFormatted->set_text(fmt::format("{:%d. %b %Y}", fmt::localtime(entry->uploaded_unix_time)));
        auto ranked = entry->GetMaxStarValue() > 0;
        bool downloaded = fcInstance->DownloadHistoryViewController->CheckIsDownloaded((std::string) entry->GetHash());

        Sombrero::FastColor songColor = Sombrero::FastColor::white();
        if (downloaded) {
            songColor = Sombrero::FastColor(0.53f, 0.53f, 0.53f, 1.0f);
        }
        // if (ranked) {
        //     songColor = UnityEngine::Color(1, 0.647f, 0, 1);
        // }

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
                bool passesFilter = DifficultyCheck(sortedDiffs[i], entry);
                diffs[i]->SetText(fmt::format("<color=#{}>{}</color>{}",
                                              (passesFilter ? "EEE" : "888"),
                                              GetCombinedShortDiffName(sortedDiffs.size(), sortedDiffs[i]),
                                              ((sortedDiffs[i]->stars > 0 && sortedDiffs[i]->diff_characteristics == song_data_core::BeatStarCharacteristics::Standard) ? fmt::format(" <color=#{}>{}", (passesFilter ? "D91" : "650"), fmt::format("{:.{}f}", sortedDiffs[i]->stars, 1)) : "")));
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

