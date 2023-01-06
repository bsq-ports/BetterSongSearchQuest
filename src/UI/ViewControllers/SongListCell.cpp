#include "UI/ViewControllers/SongListCell.hpp"
#include "UI/ViewControllers/SongList.hpp"

#include "UnityEngine/Color.hpp"
#include "sombrero/shared/FastColor.hpp"
#include "songloader/shared/API.hpp"
#include "song-details/shared/Data/MapCharacteristic.hpp"
#include "song-details/shared/Data/RankedStatus.hpp"
#include "PluginConfig.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"


DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell);

namespace BetterSongSearch::UI::ViewControllers
{
    static std::map<const SongDetailsCache::MapDifficulty, std::string> shortMapDiffNames = {
        {SongDetailsCache::MapDifficulty::Easy, "Easy"},
        {SongDetailsCache::MapDifficulty::Normal, "Norm"},
        {SongDetailsCache::MapDifficulty::Hard, "Hard"},
        {SongDetailsCache::MapDifficulty::Expert, "Ex"},
        {SongDetailsCache::MapDifficulty::ExpertPlus, "Ex+"}};

    static std::map<SongDetailsCache::MapCharacteristic, std::string> customCharNames = {
        {SongDetailsCache::MapCharacteristic::NinetyDegree, "90"},
        {SongDetailsCache::MapCharacteristic::ThreeSixtyDegree, "360"},
        {SongDetailsCache::MapCharacteristic::Lawless, "LL"},
        {SongDetailsCache::MapCharacteristic::Custom, "?"},
        {SongDetailsCache::MapCharacteristic::LightShow, "LS"}};

    std::string GetCombinedShortDiffName(int diffCount, const SongDetailsCache::SongDifficulty *diff)
    {
        auto retVal = fmt::format("{}", shortMapDiffNames[diff->difficulty]);

        if (diff->characteristic == SongDetailsCache::MapCharacteristic::Standard)
            return retVal;
        retVal += fmt::format("({})", customCharNames[diff->characteristic]);

        return retVal;
    }

    CustomSongListTableCell *CustomSongListTableCell::PopulateWithSongData(const SongDetailsCache::Song *entry)
    {
        this->levelAuthorName->set_text(entry->levelAuthorName());
        this->songLengthAndRating->set_text(fmt::format("Length: {:%M:%S} Upvotes: {}, Downvotes: {}", std::chrono::seconds(entry->songDurationSeconds), entry->upvotes, entry->downvotes));
        this->uploadDateFormatted->set_text(fmt::format("{:%d. %b %Y}", fmt::localtime(entry->uploadTimeUnix)));
        auto ranked = entry->rankedStatus == SongDetailsCache::RankedStatus::Ranked;
        bool downloaded = fcInstance->DownloadHistoryViewController->CheckIsDownloaded(entry->hash());

        Sombrero::FastColor songColor = Sombrero::FastColor::white();
        if (downloaded)
        {
            songColor = Sombrero::FastColor(0.53f, 0.53f, 0.53f, 1.0f);
        }
        // if (ranked) {
        //     songColor = UnityEngine::Color(1, 0.647f, 0, 1);
        // }

        this->fullFormattedSongName->set_text(fmt::format("{} - {}",  entry->songAuthorName(), entry->songName()));
        this->fullFormattedSongName->set_color(songColor);

        this->entry = entry;

        // auto sortedDiffs = entry->GetDifficultyVector();
        // int diffsLeft = entry->GetDifficultyVector().size();

        // for (int i = 0; i < diffs.size(); i++)
        // {
        //     bool isActive = diffsLeft != 0;

        //     diffs[i]->get_gameObject()->set_active(isActive);

        //     if (!isActive)
        //         continue;

        //     if (diffsLeft != 1 && i == diffs.size() - 1)
        //     {
        //         diffs[i]->set_text(fmt::format("<color=#0AD>{} More", diffsLeft));
        //     }
        //     else
        //     {
        //         bool passesFilter = DifficultyCheck(sortedDiffs[i], entry);
        //         diffs[i]->SetText(fmt::format("<color=#{}>{}</color>{}",
        //                                       (passesFilter ? "EEE" : "888"),
        //                                       GetCombinedShortDiffName(sortedDiffs.size(), sortedDiffs[i]),
        //                                       ((sortedDiffs[i]->stars > 0 && sortedDiffs[i]->diff_characteristics == song_data_core::BeatStarCharacteristics::Standard) ? fmt::format(" <color=#{}>{}", (passesFilter ? "D91" : "650"), fmt::format("{:.{}f}", sortedDiffs[i]->stars, 1)) : "")));
        //         diffsLeft--;
        //     }
        // }

        SetFontSizes();
        return this;
    }

    void CustomSongListTableCell::RefreshBgState()
    {
        bgContainer->set_color(UnityEngine::Color(0, 0, 0, selected || highlighted ? 0.8f : 0.45f));
    }

    void CustomSongListTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType)
    {
        RefreshBgState();
    }

    void CustomSongListTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType)
    {
        RefreshBgState();
    }

    void CustomSongListTableCell::WasPreparedForReuse()
    {
        entry = nullptr;
    }

    void CustomSongListTableCell::SetFontSizes()
    {
        bool smallFont = getPluginConfig().SmallerFontSize.GetValue();
        for (auto d : diffs)
        {
            d->set_fontSize(smallFont ? 2.5f : 2.9f);
        }

        // TODO: Different font sizes
        if (smallFont)
        {
            fullFormattedSongName->set_fontSize(2.7f);
            uploadDateFormatted->set_fontSize(2.7f);
            levelAuthorName->set_fontSize(2.3f);
            songLengthAndRating->set_fontSize(2.5f);
        }
        else
        {
            fullFormattedSongName->set_fontSize(3.2f);
            uploadDateFormatted->set_fontSize(3.2f);
            levelAuthorName->set_fontSize(2.6f);
            songLengthAndRating->set_fontSize(3.0f);
        }

        return;
    };
}
