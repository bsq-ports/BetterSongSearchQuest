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
#include "System/DateTime.hpp"
#include "System/DateTimeOffset.hpp"
#include "System/DateTimeParse.hpp"
#include "System/DateTimeResult.hpp"
#include "System/Globalization/DateTimeFormatInfo.hpp"
#include "System/Globalization/DateTimeStyles.hpp"
#include "System/Globalization/CultureInfo.hpp"
#include "System/TimeSpan.hpp"
#include "sombrero/shared/Vector2Utils.hpp"
#include "songloader/shared/API.hpp"
#include <iomanip>
#include <sstream>

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListViewController);
DEFINE_TYPE(CustomComponents, CustomCellListTableData);
DEFINE_TYPE(CustomComponents, SongListCellTableCell);
DEFINE_TYPE(CustomComponents, SongListCellData);

std::vector<const SDC_wrapper::BeatStarSong*> songList;
std::vector<const SDC_wrapper::BeatStarSong*> filteredSongList;

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
    static auto uploadFormat = il2cpp_utils::createcsstr("dd. MMM yyyy", il2cpp_utils::StringType::Manual);
    static auto lengthFormat = il2cpp_utils::createcsstr("mm\\:ss", il2cpp_utils::StringType::Manual);
    static auto culture = System::Globalization::CultureInfo::New_ctor(il2cpp_utils::createcsstr("en-US"));
    songText->set_text(il2cpp_utils::newcsstr(std::string(data->GetSongAuthor()) + " - " + std::string(data->GetName())));
    songText->set_color(RuntimeSongLoader::API::GetLevelByHash(std::string(data->GetHash())).has_value() ? UnityEngine::Color(0.53f, 0.53f, 0.53f, 1.0f) : UnityEngine::Color::get_white());
    mapperText->set_text(il2cpp_utils::newcsstr(data->GetAuthor()));

    auto result = System::DateTimeParse::Parse(il2cpp_utils::newcsstr(data->GetUploaded()), System::Globalization::DateTimeFormatInfo::get_CurrentInfo(), System::Globalization::DateTimeStyles::None);
    uploadDateText->set_text(result.ToString(uploadFormat, reinterpret_cast<System::IFormatProvider*>(culture)));

    System::TimeSpan spanDeezNutzz = System::TimeSpan::FromSeconds(data->duration_secs);
    std::string displayTime = to_utf8(csstrtostr(spanDeezNutzz.ToString(lengthFormat)));
    ratingText->set_text(il2cpp_utils::newcsstr("Length: " + displayTime + " Upvotes: " + std::to_string(data->upvotes) + " Downvotes: " + std::to_string(data->downvotes)));
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
        verticalLayoutGroup->set_padding(UnityEngine::RectOffset::New_ctor(1, 1, 1, 1));

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
        tableCell->mapperText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
        tableCell->mapperText->set_enableWordWrapping(false);

        tableCell->songText = QuestUI::BeatSaberUI::CreateText(topHoriz->get_transform(), "Deez Nuts loolollolool");
        tableCell->songText->set_fontSize(2.7f);
        tableCell->songText->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
        tableCell->songText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
        tableCell->songText->set_enableWordWrapping(false);
        
        tableCell->uploadDateText = QuestUI::BeatSaberUI::CreateText(topHoriz->get_transform(), "Failed To Parse");
        tableCell->uploadDateText->set_fontSize(2.7f);
        tableCell->uploadDateText->set_color(UnityEngine::Color(0.66f, 0.66f, 0.66f, 1));
        tableCell->uploadDateText->set_alignment(TMPro::TextAlignmentOptions::MidlineRight);
        tableCell->uploadDateText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
        tableCell->uploadDateText->set_enableWordWrapping(false);

        tableCell->ratingText = QuestUI::BeatSaberUI::CreateText(midHoriz->get_transform(), "Deez Nuts loolollolool");
        tableCell->ratingText->set_fontSize(2.5f);
        tableCell->ratingText->set_color(UnityEngine::Color(0.8f, 0.8f, 0.8f, 1));
        tableCell->ratingText->set_alignment(TMPro::TextAlignmentOptions::MidlineRight);
        tableCell->ratingText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
        tableCell->ratingText->set_enableWordWrapping(false);

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

void SortAndFilterSongs(int sort)
{
    //"Newest", "Oldest", "Ranked/Qualified time", "Most Stars", "Least Stars", "Best rated", "Worst rated", "Most Downloads"
    std::vector<std::function<bool(const SDC_wrapper::BeatStarSong*, const SDC_wrapper::BeatStarSong*)>> sortFuncs = {
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            return (struct1->uploaded_unix_time > struct2->uploaded_unix_time);
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            return (struct1->uploaded_unix_time < struct2->uploaded_unix_time);
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            int struct1UploadEpoch;
            {
                std::string s{struct1->GetUploaded()};
                std::tm t{};
                std::istringstream ss(s);

                ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
                if (ss.fail()) {
                    throw std::runtime_error{"failed to parse time string"};
                }   
                std::time_t time_stamp = mktime(&t);
                struct1UploadEpoch = time_stamp;
            }

            int struct2UploadEpoch;
            {
                std::string s{struct2->GetUploaded()};
                std::tm t{};
                std::istringstream ss(s);

                ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
                if (ss.fail()) {
                    throw std::runtime_error{"failed to parse time string"};
                }   
                std::time_t time_stamp = mktime(&t);
                struct2UploadEpoch = time_stamp;
            }
            return (struct1UploadEpoch > struct2UploadEpoch);
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            float struct1Stars = 0.0f;
            auto struct1Vector = struct1->GetDifficultyVector();
            for (int i = 0; i < struct1Vector.size(); i++)
            {
                if(struct1Vector[i]->ranked)
                    struct1Stars = std::max(struct1Vector[i]->stars, struct1Stars);
            }

            float struct2Stars = 0.0f;
            auto struct2Vector = struct2->GetDifficultyVector();
            for (int i = 0; i < struct2Vector.size(); i++)
            {
                if(struct2Vector[i]->ranked)
                    struct2Stars = std::max(struct2Vector[i]->stars, struct2Stars);
            }
            
            return struct1Stars > struct2Stars;
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            float struct1Stars = 0.0f;
            auto struct1Vector = struct1->GetDifficultyVector();
            for (int i = 0; i < struct1Vector.size(); i++)
            {
                if(struct1Vector[i]->ranked)
                    struct1Stars = std::max(struct1Vector[i]->stars, struct1Stars);
            }

            float struct2Stars = 0.0f;
            auto struct2Vector = struct2->GetDifficultyVector();
            for (int i = 0; i < struct2Vector.size(); i++)
            {
                if(struct2Vector[i]->ranked)
                    struct2Stars = std::max(struct2Vector[i]->stars, struct2Stars);
            }
            
            return struct1Stars < struct2Stars;
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            return (struct1->GetRating() > struct2->GetRating());
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            return (struct1->GetRating() < struct2->GetRating());
        },
        [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)
        {
            return (struct1->downloads > struct2->downloads);
        }
    };

    std::sort(songList.begin(), songList.end(), 
        sortFuncs[sort]
    );

    filteredSongList = songList;

    for(auto song : filteredSongList)
    {

    }
}

//SongListViewController
void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    if (!firstActivation) return;

    static ViewComponent* view;

    if (view) {
        delete view;
        view = nullptr;
    }
    std::vector<std::string> sortModes = {"Newest", "Oldest", "Ranked/Qualified time", "Most Stars", "Least Stars", "Best rated", "Worst rated", "Most Downloads"};
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
                    new StringSetting("Search " + std::to_string(songList.size()) + " songs" , "", [](StringSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("Input! %s", input.c_str());
                    }),

                    new SongListDropDown("", "Newest", sortModes, [sortModes](DropdownSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("DropDown! %s", input.c_str());
                        auto itr = std::find(sortModes.begin(), sortModes.end(), input);
                        SortAndFilterSongs(std::distance(sortModes.begin(), itr));

                        tableData->data = filteredSongList;
                        tableData->tableView->ReloadDataKeepingPosition();
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

            SortAndFilterSongs(0);

            //Make Lists
            auto list = QuestUI::BeatSaberUI::CreateScrollableCustomSourceList<CustomComponents::CustomCellListTableData*>(shitass->getTransform(), UnityEngine::Vector2(70, 6 * 11.7f));
            list->data = filteredSongList;
            list->tableView->ReloadData();
            list->get_transform()->get_parent()->SetAsFirstSibling();
            tableData = list;

            //fix scrolling lol
            GlobalNamespace::IVRPlatformHelper* mewhen;
            auto scrolls = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::ScrollView*>();
            for (size_t i = 0; i < scrolls->Length(); i++)
            {
                mewhen = scrolls->get(i)->platformHelper;
                if(mewhen != nullptr)
                    break;
            }
            for (size_t i = 0; i < scrolls->Length(); i++)
            {
                if(scrolls->get(i)->platformHelper == nullptr) scrolls->get(i)->platformHelper = mewhen;
            }
            //auto scrolls2 = this->GetComponentsInChildren<HMUI::ScrollView*>();
            //for (size_t i = 0; i < scrolls2->Length(); i++)
            //{
            //    scrolls2->get(i)->platformHelper = mewhen;
            //}

        });
    }).detach();
}