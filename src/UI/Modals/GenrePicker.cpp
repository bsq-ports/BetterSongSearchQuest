#include "UI/Modals/GenrePicker.hpp"
#include "HMUI/TableView.hpp"
#include "bsml/shared/BSML.hpp"
#include "System/Tuple_2.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "logging.hpp"
#include "assets.hpp"
#include "UI/Modals/PresetsTable.hpp"
#include "FilterOptions.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include "DataHolder.hpp"

using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::Modals, GenrePicker);


void Modals::GenrePicker::OnEnable()
{
}

void Modals::GenrePicker::PostParse()
{
}

void Modals::GenrePicker::CloseModal()
{
    this->genrePickerModal->Hide();
}

void Modals::GenrePicker::ctor()
{
    INVOKE_CTOR();
    this->initialized = false;
}

void Modals::GenrePicker::OpenModal()
{
    if (!initialized) {
        BSML::parse_and_construct(Assets::GenrePicker_bsml, this->get_transform(), this);
        initialized = true;
    }

    INFO("Opening genre picker modal");
    this->genrePickerModal->Show();

    if (!dataHolder.songDetails->tags.get_isDataAvailable()) return;

    auto fv = fcInstance->FilterViewController;
    if (!fv) return;

    dataHolder.PreprocessTags();

}

void Modals::GenrePicker::ClearGenre()
{
    dataHolder.filterOptions.mapGenreString = "";
    dataHolder.filterOptions.mapGenreExcludeString = "";
    getPluginConfig().MapGenreExcludeString.SetValue("");
    getPluginConfig().MapGenreString.SetValue("");

    // Trigger refresh of songs
    auto slController = fcInstance->SongListController;
    dataHolder.filterChanged = true;
    slController->SortAndFilterSongs(dataHolder.sort, dataHolder.search, true);

    // Update filter settings
    fcInstance->FilterViewController->UpdateLocalState();
    fcInstance->FilterViewController->ForceRefreshUI();

    this->genrePickerModal->Hide();
}