#include "UI/ViewControllers/FilterView.hpp"
#include "main.hpp"
#include "assets.hpp"
#include "bsml/shared/BSML.hpp"
#include "FilterOptions.hpp"
#include "DateUtils.hpp"
#include "UI/ViewControllers/SongList.hpp"
#include <fmt/chrono.h>
#include "Util/BSMLStuff.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"

using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI;

static const std::chrono::system_clock::time_point BEATSAVER_EPOCH_TIME_POINT{std::chrono::seconds(FilterOptions::BEATSAVER_EPOCH)};
#define coro(coroutine) GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    getLoggerOld().info("Filter View contoller activated");
    BSML::parse_and_construct(IncludedAssets::FilterView_bsml, this->get_transform(), this);

    auto maxUploadDate = GetMonthsSinceDate(FilterOptions::BEATSAVER_EPOCH);

    coro(BetterSongSearch::UI::Util::BSMLStuff::MergeSliders(this->get_gameObject()));

    #ifdef HotReload
        fileWatcher->filePath = "/sdcard/FilterView.bsml";
    #endif
}

void ViewControllers::FilterViewController::UpdateData()
{
    DEBUG("UPDATE DATA FIRED");
    
    // // Logs
    // DEBUG("\nFilter props: \n existingSongs: {} \n existingScore: {}\n rankedState: {} \n characteristic: {}\n difficulty: {} \n Required mods: {} ",
    //     (std::string ) this->existingSongs,
    //     (std::string ) this->existingScore,
    //     (std::string ) this->rankedState,
    //     (std::string ) this->characteristic,
    //     (std::string ) this->difficulty,
    //     (std::string ) this->mods
    // );
}

// Sponsors related things
void ViewControllers::FilterViewController::OpenSponsorsModal()
{
    DEBUG("OpenSponsorsModal FIRED");
}
void ViewControllers::FilterViewController::CloseSponsorModal()
{
    DEBUG("CloseSponsorModal FIRED");
}
void ViewControllers::FilterViewController::OpenSponsorsLink()
{
    DEBUG("OpenSponsorsLink FIRED");
}


// Top buttons
void ViewControllers::FilterViewController::ClearFilters()
{
    DEBUG("ClearFilters FIRED");
}
void ViewControllers::FilterViewController::ShowPresets()
{
    DEBUG("ShowPresets FIRED");
}


StringW ViewControllers::FilterViewController::DateTimeToStr(int monthsSinceFirstUpload) {
    auto val = BEATSAVER_EPOCH_TIME_POINT + std::chrono::months(int(monthsSinceFirstUpload));
    return fmt::format("{:%b:%Y}", fmt::localtime(val));
} 

// StringW ViewControllers::FilterViewController::DateTimeToStr(int d) {
//     // FilterView.hideOlderThanOptions[d].ToString("MMM yyyy", CultureInfo.InvariantCulture);
// } 