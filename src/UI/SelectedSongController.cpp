#include "UI/SelectedSongController.hpp"
#include "songloader/shared/API.hpp"
#include "UI/ViewControllers/SongList.hpp"

DEFINE_TYPE(BetterSongSearch::UI, SelectedSongController);

void BetterSongSearch::UI::SelectedSongController::SetSong(const SDC_wrapper::BeatStarSong* song)
{
    bool downloaded = RuntimeSongLoader::API::GetLevelByHash(std::string(song->GetHash())).has_value();
    playButton->get_gameObject()->SetActive(downloaded);
    downloadButton->get_gameObject()->SetActive(!downloaded);

    songNameText->set_text(il2cpp_utils::newcsstr(song->GetName()));
    authorText->set_text(il2cpp_utils::newcsstr(song->GetSongAuthor()));
}