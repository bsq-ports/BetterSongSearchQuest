#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"
using namespace BetterSongSearch::UI;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Settings/IncrementSetting.hpp"
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
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/Resources.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "HMUI/ImageView.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ScrollView.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"

#include "sombrero/shared/Vector2Utils.hpp"

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListViewController);
DEFINE_TYPE(CustomComponents, CustomCellListTableData);
DEFINE_TYPE(CustomComponents, SongListCellTableCell);
DEFINE_TYPE(CustomComponents, SongListCellData);

std::vector<const SDC_wrapper::BeatStarSong*> songList;

//SongListCellData
void CustomComponents::SongListCellData::ctor(Il2CppString* _songName, Il2CppString* _author, Il2CppString* _mapper)
{
    songName = _songName;
    author = _author;
    mapper = _mapper;
}
//SongListCellTableCell
void CustomComponents::SongListCellTableCell::ctor()
{
    selectedGroup = List<UnityEngine::GameObject*>::New_ctor();
    hoveredGroup = List<UnityEngine::GameObject*>::New_ctor();
    neitherGroup = List<UnityEngine::GameObject*>::New_ctor();
}

void CustomComponents::SongListCellTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType)
{
    getLogger().info("Selection Changed");
    RefreshVisuals();
    // if we are now selected
    if (get_selected())
    {
        QuestUI::TableView* tableView = reinterpret_cast<QuestUI::TableView*>(get_tableCellOwner());
        CustomCellListTableData* dataSource = reinterpret_cast<CustomCellListTableData*>(tableView->get_dataSource());
        //if (dataSource->listWrapper) dataSource->listWrapper->OnCellSelected(this, idx);
    }
}

void CustomComponents::SongListCellTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType)
{
    getLogger().info("Highlight Changed");
    RefreshVisuals();
}

#define UpdateGOList(list, condition) \
    int list## length = list->get_Count(); \
    for (int i = 0; i < list## length; i++) { \
        list->items->values[i]->SetActive(condition); \
     } \

void CustomComponents::SongListCellTableCell::RefreshVisuals()
{
    bool isSelected = get_selected();
    bool isHighlighted = get_highlighted(); 

    bg->background->set_color(UnityEngine::Color(0, 0, 0, isSelected ? 0.9f : isHighlighted ? 0.6f : 0.45f));

    UpdateGOList(selectedGroup, isSelected);
    UpdateGOList(hoveredGroup, isHighlighted);
    UpdateGOList(neitherGroup, !(isSelected || isHighlighted));
}

void CustomComponents::SongListCellTableCell::RefreshData(const SDC_wrapper::BeatStarSong* data)
{
    songText->set_text(il2cpp_utils::newcsstr(std::string(data->GetSongAuthor()) + " - " + std::string(data->GetName())));
    mapperText->set_text(il2cpp_utils::newcsstr(data->GetAuthor()));
    //"cache" texts
    //for (int i = 0; i < data->GetDifficultyVector().size(); i++)
    //{
    //    if(texts.size() < i)
    //    {
    //        auto shit = QuestUI::BeatSaberUI::CreateText(diffsGroup->get_transform(), "Deez Nuts loolollolool");
    //        shit->set_fontSize(2.5f);
    //        texts.push_back(shit);
    //    }
    //    texts[i]->get_gameObject()->SetActive(true);
    //    auto diff = data->GetDifficulty(i);
    //    getLogger().info("%s", std::string(diff->GetName()).c_str());
    //    texts[i]->set_text(il2cpp_utils::newcsstr(diff->GetName()));
    //}
    //if(data->GetDifficultyVector().size() < texts.size())
    //{
    //    for (int i = data->GetDifficultyVector().size(); i < texts.size(); i++)
    //    {
    //        texts[i]->get_gameObject()->SetActive(false);
    //    }
    //}
}
//CustomCellListTableData
HMUI::TableCell* CustomComponents::CustomCellListTableData::CellForIdx(HMUI::TableView* tableView, int idx)
{
    static auto QuestUICustomCellListCell_cs = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("SongListCellTableCell");
    static auto QuestUICustomTableCell_cs = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("QuestUICustomTableCell");

    CustomComponents::SongListCellTableCell* tableCell = reinterpret_cast<CustomComponents::SongListCellTableCell*>(tableView->DequeueReusableCellForIdentifier(QuestUICustomCellListCell_cs));
    if(!tableCell)
    {
        tableCell = UnityEngine::GameObject::New_ctor()->AddComponent<CustomComponents::SongListCellTableCell*>();
        tableCell->set_reuseIdentifier(QuestUICustomCellListCell_cs);
        tableCell->set_name(QuestUICustomTableCell_cs);

        tableCell->get_gameObject()->AddComponent<HMUI::Touchable*>();
        tableCell->set_interactable(true);

        auto fitter = tableCell->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        fitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);

        auto verticalLayoutGroup = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(tableCell->get_transform());
        tableCell->bg = verticalLayoutGroup->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        tableCell->bg->ApplyBackgroundWithAlpha(il2cpp_utils::newcsstr("round-rect-panel"), 0.6f);

        auto layout = tableCell->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layout->set_preferredHeight(11.75f);
        layout->set_preferredWidth(70);

        tableCell->get_transform()->SetParent(tableView->get_transform()->GetChild(0)->GetChild(0), false);

        auto topHoriz = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());
        //auto topfitter = topHoriz->get_gameObject()->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
        //topfitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        //topfitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

        auto midHoriz = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());
        //auto midfitter = midHoriz->get_gameObject()->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
        //midfitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        //midfitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

        auto bottomHoriz = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());
        auto bottomfitter = bottomHoriz->get_gameObject()->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
        bottomfitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        bottomfitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

        auto bg = bottomHoriz->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        bg->ApplyBackgroundWithAlpha(il2cpp_utils::newcsstr("round-rect-panel"), 1.0f);
        bg->background->set_color(UnityEngine::Color::get_black());

        bottomHoriz->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>()->set_preferredHeight(3.9f);
        bottomHoriz->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(68.0f);

        tableCell->mapperText = QuestUI::BeatSaberUI::CreateText(midHoriz->get_transform(), "Deez Nuts loolollolool");
        tableCell->mapperText->set_fontSize(2.3f);
        tableCell->mapperText->set_color(UnityEngine::Color(0.8f, 0.8f, 0.8f, 1));
        tableCell->mapperText->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);

        tableCell->songText = QuestUI::BeatSaberUI::CreateText(topHoriz->get_transform(), "Deez Nuts loolollolool");
        tableCell->songText->set_fontSize(2.7f);
        tableCell->songText->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);

        tableCell->diffsGroup = bottomHoriz->get_gameObject();
    }
    tableCell->RefreshData(data[idx]);
    return tableCell;
}

float CustomComponents::CustomCellListTableData::CellSize()
{
    return 11.7f;
}
int CustomComponents::CustomCellListTableData::NumberOfCells()
{
    return data.size();
}
//SongListViewController
void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    static ViewComponent* view;

    std::vector<std::string> sortModes = {"Newest", "Oldest", "Ranked/Qualified time", "Most Stars", "Least Stars", "Best rated", "Worst rated", "Worst local score", "Most Downloads"};

    if (view) {
        delete view;
        view = nullptr;
    }
    SongListHorizontalLayout* shitass;

    // async ui because this causes lag spike
    std::thread([this, sortModes, &shitass]{
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

                    new SongListDropDown("", "Newest", sortModes, [](DropdownSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("DropDown! %s", input.c_str());
                    }),
                }),
                (new SongListHorizontalLayout({
                    new VerticalLayoutGroup({
                        new Text("Song Details"),
                    })
                }))->with([&shitass](SongListHorizontalLayout* scrollableContainer)
                    {
                        shitass = scrollableContainer;
                    })
            })
        });
        QuestUI::MainThreadScheduler::Schedule([this, shitass]{
            view->render();
            
            //fix scrolling lol
            GlobalNamespace::IVRPlatformHelper* mewhen;
            auto scrolls = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::ScrollView*>();
            for (size_t i = 0; i < scrolls->Length(); i++)
            {
                mewhen = scrolls->get(i)->platformHelper;
                if(mewhen != nullptr)
                    break;
            }
            auto scrolls2 = this->GetComponentsInChildren<HMUI::ScrollView*>();
            for (size_t i = 0; i < scrolls2->Length(); i++)
            {
                scrolls2->get(i)->platformHelper = mewhen;
            }

            //Make Lists
            auto list = QuestUI::BeatSaberUI::CreateScrollableCustomSourceList<CustomComponents::CustomCellListTableData*>(shitass->getTransform(), UnityEngine::Vector2(70, 6 * 11.7f));
            list->data = songList;
            list->tableView->ReloadData();
            list->get_transform()->get_parent()->SetAsFirstSibling();
        });
    }).detach();
}