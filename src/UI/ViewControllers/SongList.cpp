#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "CustomComponents.hpp"
#include "questui_components/shared/components/ViewComponent.hpp"
#include "questui_components/shared/components/Text.hpp"
#include "questui_components/shared/components/ScrollableContainer.hpp"
#include "questui_components/shared/components/HoverHint.hpp"
#include "questui_components/shared/components/Button.hpp"
#include "questui_components/shared/components/Modal.hpp"
#include "questui_components/shared/components/list/CustomCelledList.hpp"
#include "questui_components/shared/components/layouts/VerticalLayoutGroup.hpp"
#include "questui_components/shared/components/layouts/HorizontalLayoutGroup.hpp"
#include "questui_components/shared/components/settings/ToggleSetting.hpp"
#include "questui_components/shared/components/settings/StringSetting.hpp"
#include "questui_components/shared/components/settings/IncrementSetting.hpp"
#include "questui_components/shared/components/settings/DropdownSetting.hpp"
using namespace QuestUI_Components;
#include "UnityEngine/UI/ContentSizeFitter.hpp"
using namespace QuestUI;

#include "config-utils/shared/config-utils.hpp"

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListViewController);

UnityEngine::UI::VerticalLayoutGroup* songListLayout;

void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    static ViewComponent* view;

    std::vector<std::string> sortModes = {"Newest", "Oldest", "Ranked/Qualified time", "Most Stars", "Least Stars", "Best rated", "Worst rated", "Worst local score", "Most Downloads"};

    if (view) {
        delete view;
        view = nullptr;
    }

    // async ui because this causes lag spike
    std::thread([this, sortModes]{
        view = new ViewComponent(this->get_transform(), {
            new SongListVerticalLayoutGroup({
                new SongListHorizontalFilterBar({
                    new Button("RANDOM", [](Button* button, UnityEngine::Transform* parentTransform){

                    }),
                    new Button("MULTI", [](Button* button, UnityEngine::Transform* parentTransform){

                    }),
                    new StringSetting("Search", "", [](StringSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("Input! %s", input.c_str());
                    }),

                    new DropdownSetting("", "Newest", sortModes, [](DropdownSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("DropDown! %s", input.c_str());
                    }),
                }),
                new SongListHorizontalLayout({
                    new ScrollableContainer({
                        //CUSOTM LIST GO HERE
                    }),
                    new VerticalLayoutGroup({
                        new Text("Song Details"),
                    })
                })
            })
        });

        QuestUI::MainThreadScheduler::Schedule([]{
            view->render();
        });
    }).detach();
}