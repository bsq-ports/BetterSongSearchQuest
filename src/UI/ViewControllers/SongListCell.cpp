#include "UI/ViewControllers/SongListCell.hpp"
#include "UI/ViewControllers/SongList.hpp"

#include "UnityEngine/Color.hpp"
#include "sombrero/shared/FastColor.hpp"
#include "songloader/shared/API.hpp"
#include "song-details/shared/Data/MapCharacteristic.hpp"
#include "song-details/shared/Data/RankedStatus.hpp"
#include "PluginConfig.hpp"
#include "Util/SongUtil.hpp"
#include <fmt/chrono.h>
#include <fmt/core.h>
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"


using namespace BetterSongSearch::Util;

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, CustomSongListTableCell);

struct DiffIndex {
    const SongDetailsCache::SongDifficulty* diff;
    bool passesFilter;
    float stars;
};

namespace BetterSongSearch::UI::ViewControllers
{
    static std::map<const SongDetailsCache::MapDifficulty, std::string> shortMapDiffNames = {
        {SongDetailsCache::MapDifficulty::Easy, "Easy"},
        {SongDetailsCache::MapDifficulty::Normal, "Norm"},
        {SongDetailsCache::MapDifficulty::Hard, "Hard"},
        {SongDetailsCache::MapDifficulty::Expert, "Ex"},
        {SongDetailsCache::MapDifficulty::ExpertPlus, "Ex+"}};

    static std::map<SongDetailsCache::MapCharacteristic, std::string> customCharNames = {
        {SongDetailsCache::MapCharacteristic::Standard, ""},
        {SongDetailsCache::MapCharacteristic::NinetyDegree, "90"},
        {SongDetailsCache::MapCharacteristic::OneSaber, "OS"},
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

        this->fullFormattedSongName->set_text(fmt::format("{} - {}",  entry->songAuthorName(), entry->songName()));
        this->fullFormattedSongName->set_color(songColor);

        this->entry = entry;

        std::vector<DiffIndex>  sortedDiffs;
        for (const SongDetailsCache::SongDifficulty &diff: *entry) {
            sortedDiffs.push_back({
                &diff,
                DifficultyCheck(&diff, entry),
                getStars(&diff)
            });
        };

        
        // TODO: Actually sort diffs..
        std::stable_sort(sortedDiffs.begin(), sortedDiffs.end(), [entry](const DiffIndex& a,  const DiffIndex& b) {
           
            auto diff1 = (a.passesFilter? 1:-3) + (a.diff->characteristic == SongDetailsCache::MapCharacteristic::Standard? 1:0);
            auto diff2 = (b.passesFilter? 1:-3) + (b.diff->characteristic == SongDetailsCache::MapCharacteristic::Standard? 1:0);

            // Descending
            return diff1 > diff2;
        });

        // If most stars
        if (DataHolder::currentSort == SortMode::Most_Stars) {
            std::stable_sort(sortedDiffs.begin(), sortedDiffs.end(), [entry](const DiffIndex& a,  const DiffIndex& b) {
           
                auto diff1 = - a.stars;
                auto diff2 = - b.stars;

                // Ascending
                return diff1 < diff2;
            });
        }
        // If least stars
        if (DataHolder::currentSort == SortMode::Least_Stars) {
            std::stable_sort(sortedDiffs.begin(), sortedDiffs.end(), [entry](const DiffIndex& a,  const DiffIndex& b) {
                auto diff1 = a.stars > 0 ? a.stars: -420.0f;
                auto diff2 = b.stars > 0 ? b.stars: -420.0f;
                // Ascending
                return diff1 < diff2;
            });
        }

        // By ranked
        std::stable_sort(sortedDiffs.begin(), sortedDiffs.end(), [entry](const DiffIndex& a,  const DiffIndex& b) {
            auto diff1 = a.stars > 0 ? 1: 0;
            auto diff2 = b.stars > 0 ? 1: 0;
            // Descending
            return diff1 < diff2;
        });

    
     

        int diffsLeft = sortedDiffs.size();

        for (int i = 0; i < diffs.size(); i++)
        {
            bool isActive = diffsLeft != 0;
            diffs[i]->get_gameObject()->set_active(isActive);
            if (!isActive)
                continue;
            if (diffsLeft != 1 && i == diffs.size() - 1)
            {
                diffs[i]->set_text(fmt::format("<color=#0AD>{} More", diffsLeft));
            }
            else
            {
                bool passesFilter = sortedDiffs[i].passesFilter;
                auto diffname = GetCombinedShortDiffName(entry->diffCount, sortedDiffs[i].diff);
                std::string stars = ""; 
                if (sortedDiffs[i].stars > 0 && sortedDiffs[i].diff->characteristic == SongDetailsCache::MapCharacteristic::Standard) {
                    stars = fmt::format(" <color=#{}>{}", (passesFilter ? "D91" : "650"), fmt::format("{:.{}f}", sortedDiffs[i].stars, 1));
                };
                
                diffs[i]->SetText(
                    fmt::format("<color=#{}>{}</color>{}",
                        (passesFilter ? "EEE" : "888"),
                        diffname,
                        stars
                    )
                );
                diffsLeft--;
            }
        }

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
