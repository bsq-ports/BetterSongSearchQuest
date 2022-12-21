#include "UI/Modals/MultiDL.hpp"
#include "main.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "bsml/shared/BSML.hpp"
#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"

#include "assets.hpp"
#include "Util/CurrentTimeMs.hpp"
#include "UI/ViewControllers/DownloadListTableData.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

using namespace QuestUI;
using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::Modals, MultiDL);

void Modals::MultiDL::OnEnable()
{
}

void Modals::MultiDL::StartMultiDownload()
{   
    this->CloseModal();
    DEBUG("Downloading {} songs", this->songsToDownload);
    return;
}
void Modals::MultiDL::CloseModal()
{
    this->modal->Hide();
}

void Modals::MultiDL::ctor()
{
    INVOKE_CTOR();
    this->initialized = false;
    this->songsToDownload = 5;
}

void Modals::MultiDL::OpenModal()
{
    if (!initialized) {
        BSML::parse_and_construct(IncludedAssets::MultiDl_bsml, this->get_transform(), this);
        initialized = true;
    }
    this->modal->Show();
}
