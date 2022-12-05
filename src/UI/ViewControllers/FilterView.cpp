#include "main.hpp"
#include "assets.hpp"
#include "UI/ViewControllers/FilterView.hpp"
#include "bsml/shared/BSML.hpp"
using namespace BetterSongSearch::UI;


DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, FilterViewController);

void ViewControllers::FilterViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling)
{
    if (!firstActivation)
        return;

    getLoggerOld().info("Filter View contoller activated");
    BSML::parse_and_construct(IncludedAssets::FilterView_bsml, this->get_transform(), this);
}