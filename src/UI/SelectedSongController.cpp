#include "main.hpp"
#include "UI/SelectedSongController.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "questui/shared/ArrayUtil.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "UnityEngine/Resources.hpp"

DEFINE_TYPE(BetterSongSearch::UI, SelectedSongController);

void BetterSongSearch::UI::SelectedSongController::SetSong(const SDC_wrapper::BeatStarSong* song)
{
    auto beatmap = RuntimeSongLoader::API::GetLevelByHash(std::string(song->GetHash()));
    bool downloaded = beatmap.has_value();
    if(downloaded)
    {
        coverImage->set_sprite(beatmap.value()->coverImage);
    }
    else
    {
        getLogger().info("Downloading");
        coverImage->set_sprite(defaultImage);
        //):
        BeatSaver::API::GetBeatmapByHashAsync(std::string(song->GetHash()), 
        [this](std::optional<BeatSaver::Beatmap> beatmap)
        {
            BeatSaver::API::GetCoverImageAsync(beatmap.value(), 
            [this](std::vector<uint8_t> result) {
                auto arr = il2cpp_utils::vectorToArray(result);
                getLogger().info("Downloaded");
                QuestUI::MainThreadScheduler::Schedule([this, arr]
                {
                    this->coverImage->set_sprite(QuestUI::BeatSaberUI::ArrayToSprite(arr));
                });
            });
        });
    }
    playButton->get_gameObject()->SetActive(downloaded);
    downloadButton->get_gameObject()->SetActive(!downloaded);

    songNameText->set_text(il2cpp_utils::newcsstr(song->GetName()));
    authorText->set_text(il2cpp_utils::newcsstr(song->GetSongAuthor()));
}