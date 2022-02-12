#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "CustomComponents.hpp"
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


#include "config-utils/shared/config-utils.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Events/UnityAction.hpp"

#include "HMUI/Touchable.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "HMUI/VerticalScrollIndicator.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "System/StringComparison.hpp"
#include "System/Action_2.hpp"
#include <iomanip>
#include <sstream>

#include <chrono>
#include <string>
#include <algorithm>
#include <functional>

using namespace BetterSongSearch::UI;
using namespace QuestUI;
using namespace QUC;

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListViewController);

// Source
DEFINE_QUC_CUSTOMLIST_TABLEDATA(BetterSongSearch::UI, QUCObjectTableData);
DEFINE_QUC_CUSTOMLIST_CELL(BetterSongSearch::UI, QUCObjectTableCell)

void BetterSongSearch::UI::QUCObjectTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType)
{
    RefreshVisuals();
}

void QUCObjectTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
    RefreshVisuals();
}

void QUCObjectTableCell::render(CellData const &cellData, RenderContext &ctx, CellComponent &cellComp) {
    this->cellComp = &cellComp;
}

void QUCObjectTableCell::RefreshVisuals() {
    if (cellComp) {
        bool isSelected = get_selected();
        bool isHighlighted = get_highlighted();

        cellComp->setBgColor(UnityEngine::Color(0, 0, 0, isSelected ? 0.9f : isHighlighted ? 0.6f : 0.45f));
    }
}

// this hurts
std::vector<std::string> split(std::string_view buffer, const std::string_view delimeter = " ") {
    std::vector<std::string> ret{};
    std::decay_t<decltype(std::string::npos)> pos{};
    while ((pos = buffer.find(delimeter)) != std::string::npos) {
        const auto match = buffer.substr(0, pos);
        if (!match.empty()) ret.emplace_back(match);
        buffer = buffer.substr(pos + delimeter.size());
    }
    if (!buffer.empty()) ret.emplace_back(buffer);
    return ret;
}

std::string removeSpecialCharacter(std::string_view const s) {
    std::string stringy(s);
    for (int i = 0; i < stringy.size(); i++) {

        if (stringy[i] < 'A' || stringy[i] > 'Z' &&
            stringy[i] < 'a' || stringy[i] > 'z')
        {
            stringy.erase(i, 1);
            i--;
        }
    }
    return stringy;
}

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

inline std::string toLower(std::string_view s) {
    return toLower(std::string(s));
}

inline std::string toLower(char const* s) {
    return toLower(std::string(s));
}

bool deezContainsDat(const SDC_wrapper::BeatStarSong* song, std::vector<std::string> searchTexts)
{
    int words = 0;
    int matches = 0;
    std::string songName = removeSpecialCharacter(toLower(song->GetName()));
    std::string songSubName = removeSpecialCharacter(toLower(song->GetSubName()));
    std::string songAuthorName = removeSpecialCharacter(toLower(song->GetSongAuthor()));
    std::string levelAuthorName = removeSpecialCharacter(toLower(song->GetAuthor()));
    std::string songKey = toLower(song->key.string_data);

    for (int i = 0; i < searchTexts.size(); i++)
    {
        words++;
        std::string searchTerm = toLower(searchTexts[i]);
        if (i == searchTexts.size() - 1)
        {
            searchTerm.resize(searchTerm.length()-1);
        }


        if (songName.find(searchTerm) != std::string::npos ||
            songSubName.find(searchTerm) != std::string::npos ||
            songAuthorName.find(searchTerm) != std::string::npos ||
            levelAuthorName.find(searchTerm) != std::string::npos ||
            songKey.find(searchTerm) != std::string::npos)
        {
            matches++;
        }
    }

    return matches == words;
}
std::string prevSearch = "";
int prevSort = 0;
FilterOptions* filterOptions = new FilterOptions();
BetterSongSearch::UI::ViewControllers::SongListViewController* songListController;

bool MeetsFilter(const SDC_wrapper::BeatStarSong* song)
{
    bool downloaded = DataHolder::downloadedSongList.contains(song);
    if(downloaded)
    {
        if(filterOptions->downloadType == FilterOptions::DownloadFilterType::HideDownloaded)
            return false;
    }
    else
    {
        if(filterOptions->downloadType == FilterOptions::DownloadFilterType::OnlyDownloaded)
            return false;
    }

    bool ranked = song->GetMaxStarValue() > 0;
    if(ranked)
    {
        if(filterOptions->rankedType == FilterOptions::RankedFilterType::HideRanked)
            return false;
    }
    else
    {
        if(filterOptions->rankedType == FilterOptions::RankedFilterType::OnlyRanked)
            return false;
    }

    float minNPS = 500000, maxNPS = 0;
    float minNJS = 500000, maxNJS = 0;
    float minStars = 500000, maxStars = 0;

    bool foundValidDiff = false;
    bool foundValidChar = false;

    for(auto diff : song->GetDifficultyVector())
    {
        float nps = (float)diff->notes / (float)song->duration_secs;
        float njs = diff->njs;
        float stars = diff->stars;

        minNPS = std::min(nps, minNPS);
        maxNPS = std::max(nps, maxNPS);

        minNJS = std::min(njs, minNJS);
        maxNJS = std::max(njs, maxNJS);
        
        minStars = std::min(stars, minStars);
        maxStars = std::max(stars, maxStars);
        switch(filterOptions->difficultyFilter)
        {
            case FilterOptions::DifficultyFilterType::All:
                foundValidDiff = true;
                break;
            case FilterOptions::DifficultyFilterType::Easy:
                if(diff->GetName() == "Easy") foundValidDiff = true;
                break;
            case FilterOptions::DifficultyFilterType::Normal:
                if(diff->GetName() == "Normal") foundValidDiff = true;
                break;
            case FilterOptions::DifficultyFilterType::Hard:
                if(diff->GetName() == "Hard") foundValidDiff = true;
                break;
            case FilterOptions::DifficultyFilterType::Expert:
                if(diff->GetName() == "Expert") foundValidDiff = true;
                break;
            case FilterOptions::DifficultyFilterType::ExpertPlus:
                if(diff->GetName() == "ExpertPlus") foundValidDiff = true;
                break;
        }
        switch(filterOptions->charFilter)//"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
        {
            case FilterOptions::CharFilterType::All:
                foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::Custom:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::Unknown) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::Standard:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::Standard) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::OneSaber:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::OneSaber) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::NoArrows:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::NoArrows) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::NinetyDegrees:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::Degree90) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::ThreeSixtyDegrees:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::Degree360) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::LightShow:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::Lightshow) foundValidChar = true;
                break;
            case FilterOptions::CharFilterType::Lawless:
                if(diff->diff_characteristics == song_data_core::BeatStarCharacteristics::Lawless) foundValidChar = true;
                break;
        }
    }
    if(!foundValidDiff) return false;
    if(!foundValidChar) return false;

    if(minNPS < filterOptions->minNPS) return false;
    if(maxNPS > filterOptions->maxNPS) return false;
    if(minNJS < filterOptions->minNJS) return false;
    if(maxNJS > filterOptions->maxNJS) return false;
    if(minStars < filterOptions->minStars) return false;
    if(maxStars > filterOptions->maxStars) return false;


    return true;
}

void SortAndFilterSongs(int sort, std::string_view const search)
{
    if(songListController != nullptr)
    {
        songListController->tablePtr->tableView->ClearSelection();
        songListController->tablePtr->tableView->ClearHighlights();
    }
    prevSort = sort;
    prevSearch = search;
    // std::thread([&]{
        std::vector<std::function<bool(const SDC_wrapper::BeatStarSong*, const SDC_wrapper::BeatStarSong*)>> sortFuncs = {
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Newest
            {
                return (struct1->uploaded_unix_time > struct2->uploaded_unix_time);
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Oldest
            {
                return (struct1->uploaded_unix_time < struct2->uploaded_unix_time);
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Latest Ranked
            {
                int struct1RankedUpdateTime;
                auto struct1DiffVec = struct1->GetDifficultyVector();
                for(int i = 0; i < struct1DiffVec.size(); i++)
                {
                    struct1RankedUpdateTime = std::max((int)struct1DiffVec[i]->ranked_update_time_unix_epoch, struct1RankedUpdateTime);
                }

                int struct2RankedUpdateTime;
                auto struct2DiffVec = struct2->GetDifficultyVector();
                for(int i = 0; i < struct2DiffVec.size(); i++)
                {
                    struct2RankedUpdateTime = std::max((int)struct2DiffVec[i]->ranked_update_time_unix_epoch, struct2RankedUpdateTime);
                }
                return (struct1RankedUpdateTime > struct2RankedUpdateTime);
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Most Stars
            {
                return struct1->GetMaxStarValue() > struct2->GetMaxStarValue();
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Least Stars
            {
                return struct1->GetMaxStarValue() < struct2->GetMaxStarValue();
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Best rated
            {
                return (struct1->rating > struct2->rating);
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Worst rated
            {
                return (struct1->rating > struct2->rating);
            },
            [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Most Downloads
            {
                return (struct1->downloads > struct2->downloads);
            }
        };
        //This is so unranked songs dont show up in the Least Stars Sort
        std::vector<std::function<bool(const SDC_wrapper::BeatStarSong*)>> sortFilterFuncs = {
            [] (const SDC_wrapper::BeatStarSong* song)//Newest
            {
                return true;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Oldest
            {
                return true;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Latest Ranked
            {
                auto ranked = song->GetMaxStarValue() > 0;
                return ranked;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Most Stars
            {
                auto ranked = song->GetMaxStarValue() > 0;
                return ranked;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Least Stars
            {
                auto ranked = song->GetMaxStarValue() > 0;
                return ranked;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Best rated
            {
                return true;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Worst rated
            {
                return true;
            },
            [] (const SDC_wrapper::BeatStarSong* song)//Most Downloads
            {
                return true;
            }
        };
        //filteredSongList = songList;
        DataHolder::filteredSongList.clear();
        for(auto song : DataHolder::songList)
        {
            if(deezContainsDat(song, split(search, " ")) && sortFilterFuncs[sort](song) && MeetsFilter(song))
            {
                DataHolder::filteredSongList.emplace_back(song);
            }
        }
        std::sort(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end(),
            sortFuncs[sort]
        );
    //}).detach();
}

void Sort()
{
    SortAndFilterSongs(prevSort, prevSearch);
    std::vector<CellData> filteredCells(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());
    songListController->table.child.getStatefulVector(songListController->ctx) = std::move(filteredCells);
    songListController->table.update();
}



auto SelectedSongControllerLayout(ViewControllers::SongListViewController* view) {
    auto defaultImage = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>().First([](UnityEngine::Sprite* x) {return x->get_name() == "CustomLevelsPack"; });

    auto& controller = view->selectedSongController;
    controller.child.defaultImage = defaultImage;

    detail::VerticalLayoutGroup selectedSongView(QUC::detail::refComp(controller));

    ModifyLayout layout(selectedSongView);
    layout.childForceExpandHeight = false;
    layout.padding = {2,2,2,2};

    ModifyLayoutElement layoutElement(layout);
    layoutElement.preferredWidth = 40;

    std::vector<CellData> filteredSongs(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());

    view->table.child.initCellDatas = filteredSongs;

    return QUC::Backgroundable("round-rect-panel", true,
        SongListHorizontalLayout(
                selectedSongView,
                OnRenderCallback(
                    QUC::detail::refComp(view->table),
                    [view](auto& self, RenderContext &ctx, RenderContextChildData& data) {
                        auto& tableState = data.getData<decltype(ViewControllers::SongListViewController::TableType::child)::RenderState>();

                        view->tablePtr = tableState.dataSource;
                    }
                )
       )
    );
}

//SongListViewController
void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    songListController = this;
    if (!firstActivation) return;

    std::array<std::string, 8> sortModes(
            {"Newest", "Oldest", "Latest Ranked", "Most Stars", "Least Stars", "Best rated", "Worst rated",
             "Most Downloads"});




    SortAndFilterSongs(0, "");

    static auto songListControllerView = SongListVerticalLayoutGroup(
            SongListHorizontalFilterBar(
                    Button("RANDOM", [](Button &button, UnityEngine::Transform *transform, RenderContext &ctx) {
                        int random = rand() % DataHolder::filteredSongList.size();
                        songListController->tablePtr->tableView->ScrollToCellWithIdx(random, HMUI::TableView::ScrollPositionType::Center, true);
                    }),
                    Button("MULTI", nullptr),
                    StringSetting("Search by Song, Key, Mapper...",
                                  [](StringSetting &, std::string const &input, UnityEngine::Transform *,
                                     RenderContext &ctx) {
                                      getLogger().debug("Input! %s", input.c_str());
                                      SortAndFilterSongs(prevSort, input);

                                      songListController->table.child.getStatefulVector(songListController->ctx) = std::vector<CellData>(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());;
                                      songListController->table.update();
                                  }),
                    SongListDropDown<std::tuple_size_v<decltype(sortModes)>>("", "Newest", [sortModes](
                            DropdownSetting<std::tuple_size_v<decltype(sortModes)>> &, std::string const &input,
                            UnityEngine::Transform *, RenderContext &ctx) {
                        getLogger().debug("DropDown! %s", input.c_str());
                        auto itr = std::find(sortModes.begin(), sortModes.end(), input);
                        SortAndFilterSongs(std::distance(sortModes.begin(), itr), prevSearch);

                        songListController->table.child.getStatefulVector(songListController->ctx) = std::vector<CellData>(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());;
                        songListController->table.update();
                    }, sortModes)
            ),
            SelectedSongControllerLayout(this)
    );



    if (firstActivation) {
        this->ctx = RenderContext(get_transform());

        detail::renderSingle(songListControllerView, ctx);

        //Make Lists

        auto click = std::function([=](HMUI::TableView *tableView, int row) {
            this->selectedSongController.child.SetSong(DataHolder::filteredSongList[row]);
        });
        auto yes = il2cpp_utils::MakeDelegate<System::Action_2<HMUI::TableView *, int> *>(classof(System::Action_2<HMUI::TableView *, int>*), click);

        tablePtr->tableView->add_didSelectCellWithIdxEvent(yes);
        tablePtr->get_transform()->get_parent()->SetAsFirstSibling();
    } else {
        detail::renderSingle(songListControllerView, ctx);
    }




    //fix scrolling lol
    GlobalNamespace::IVRPlatformHelper *mewhen;
    auto scrolls = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::ScrollView *>();
    for (int i = 0; i < scrolls.Length(); i++) {
        mewhen = scrolls.get(i)->platformHelper;
        if (mewhen != nullptr)
            break;
    }
    for (int i = 0; i < scrolls.Length(); i++) {
        if (scrolls.get(i)->platformHelper == nullptr) scrolls.get(i)->platformHelper = mewhen;
    }
}