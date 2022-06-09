#include "UI/ViewControllers/SongList.hpp"
#include "main.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "CustomComponents.hpp"
#include "questui_components/shared/components/Button.hpp"
#include "questui_components/shared/components/list/CustomCelledList.hpp"
#include "questui_components/shared/components/layouts/VerticalLayoutGroup.hpp"
#include "questui_components/shared/components/layouts/HorizontalLayoutGroup.hpp"
#include "questui_components/shared/components/settings/StringSetting.hpp"
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



std::string prevSearch;
SortMode prevSort = (SortMode) 0;
BetterSongSearch::UI::ViewControllers::SongListViewController* songListController;

using SortFunction = std::function<bool(SDC_wrapper::BeatStarSong const*, SDC_wrapper::BeatStarSong const*)>;

static const std::unordered_map<SortMode, SortFunction> sortFunctionMap = {
        {SortMode::Newest, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Newest
                           {
                               return (struct1->uploaded_unix_time > struct2->uploaded_unix_time);
                           }},
        {SortMode::Oldest, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Oldest
                           {
                               return (struct1->uploaded_unix_time < struct2->uploaded_unix_time);
                           }},
        {SortMode::Latest_Ranked, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Latest Ranked
                           {
                               int struct1RankedUpdateTime;
                               auto struct1DiffVec = struct1->GetDifficultyVector();
                               for(auto const& i : struct1DiffVec)
                               {
                                   struct1RankedUpdateTime = std::max((int)i->ranked_update_time_unix_epoch, struct1RankedUpdateTime);
                               }

                               int struct2RankedUpdateTime;
                               auto struct2DiffVec = struct2->GetDifficultyVector();
                               for(auto const& i : struct2DiffVec)
                               {
                                   struct2RankedUpdateTime = std::max((int)i->ranked_update_time_unix_epoch, struct2RankedUpdateTime);
                               }
                               return (struct1RankedUpdateTime > struct2RankedUpdateTime);
                           }},
        {SortMode::Most_Stars, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Most Stars
                           {
                               return struct1->GetMaxStarValue() > struct2->GetMaxStarValue();
                           }},
        {SortMode::Least_Stars, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Least Stars
                           {
                               return struct1->GetMaxStarValue() < struct2->GetMaxStarValue();
                           }},
        {SortMode::Best_rated, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Best rated
                           {
                               return (struct1->rating > struct2->rating);
                           }},
        {SortMode::Worst_rated, [] (const SDC_wrapper::BeatStarSong* struct1, const SDC_wrapper::BeatStarSong* struct2)//Worst rated
                           {
                               return (struct1->rating < struct2->rating);
                           }}
};

//This is so unranked songs dont show up in the Least Stars Sort
static const std::function<bool(const SDC_wrapper::BeatStarSong* song)> rankedFilterFunction = [] (const SDC_wrapper::BeatStarSong* song)//Least Stars
{
    auto ranked = song->GetMaxStarValue() > 0;
    return ranked;
};


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
    if (cellComp && cellComp->setBgColor) {
        bool isSelected = get_selected();
        bool isHighlighted = get_highlighted();

        CRASH_UNLESS(cellComp->setBgColor);
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

bool DifficultyCheck(const SDC_wrapper::BeatStarSongDifficultyStats* diff, const SDC_wrapper::BeatStarSong* song) {
    auto const& currentFilter = DataHolder::filterOptions;


    if(diff->stars < currentFilter.minStars || diff->stars > currentFilter.maxStars)
        return false;

    switch(currentFilter.difficultyFilter)
    {
        case FilterOptions::DifficultyFilterType::All:
            break;
        case FilterOptions::DifficultyFilterType::Easy:
            if(diff->GetName() != "Easy") return false;
            break;
        case FilterOptions::DifficultyFilterType::Normal:
            if(diff->GetName() != "Normal") return false;
            break;
        case FilterOptions::DifficultyFilterType::Hard:
            if(diff->GetName() != "Hard") return false;
            break;
        case FilterOptions::DifficultyFilterType::Expert:
            if(diff->GetName() != "Expert") return false;
            break;
        case FilterOptions::DifficultyFilterType::ExpertPlus:
            if(diff->GetName() != "ExpertPlus") return false;
            break;
    }

    switch(currentFilter.charFilter) //"Any", "Custom", "Standard", "One Saber", "No Arrows", "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
    {
        case FilterOptions::CharFilterType::All:
            break;
        case FilterOptions::CharFilterType::Custom:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Unknown) return false;
            break;
        case FilterOptions::CharFilterType::Standard:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Standard) return false;
            break;
        case FilterOptions::CharFilterType::OneSaber:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::OneSaber) return false;
            break;
        case FilterOptions::CharFilterType::NoArrows:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::NoArrows) return false;
            break;
        case FilterOptions::CharFilterType::NinetyDegrees:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Degree90) return false;
            break;
        case FilterOptions::CharFilterType::ThreeSixtyDegrees:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Degree360) return false;
            break;
        case FilterOptions::CharFilterType::LightShow:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Lightshow) return false;
            break;
        case FilterOptions::CharFilterType::Lawless:
            if(diff->diff_characteristics != song_data_core::BeatStarCharacteristics::Lawless) return false;
            break;
    }

    if(diff->njs < currentFilter.minNJS || diff->njs > currentFilter.maxNJS)
        return false;

    if(currentFilter.rankedType == FilterOptions::RankedFilterType::OnlyRanked && !diff->ranked)
        return false;

    // do mods check here

    if(song->duration_secs > 0) {
        float nps = (float)diff->notes / (float)song->duration_secs;

        if(nps < currentFilter.minNPS || nps > currentFilter.maxNPS)
            return false;
    }

    return true;
}

bool MeetsFilter(const SDC_wrapper::BeatStarSong* song)
{
    auto const& filterOptions = DataHolder::filterOptions;

    /*if(filterOptions->uploaders.size() != 0) {
		if(std::find(filterOptions->uploaders.begin(), filterOptions->uploaders.end(), removeSpecialCharacter(toLower(song->GetAuthor()))) != filterOptions->uploaders.end()) {
		} else {
			return false;
		}
	}*/

    if(song->GetRating() < filterOptions.minRating) return false;
    if(((int)song->upvotes + (int)song->downvotes) < filterOptions.minVotes) return false;

    bool downloaded = DataHolder::downloadedSongList.contains(song);
    if(downloaded)
    {
        if(filterOptions.downloadType == FilterOptions::DownloadFilterType::HideDownloaded)
            return false;
    }
    else
    {
        if(filterOptions.downloadType == FilterOptions::DownloadFilterType::OnlyDownloaded)
            return false;
    }

    bool passesFilter = true;

    for(auto diff : song->GetDifficultyVector())
    {
        if(DifficultyCheck(diff, song)) {
            passesFilter = true;
            break;
        }
        else
            passesFilter = false;
    }

    if(!passesFilter)
        return false;

    if(song->duration_secs < filterOptions.minLength) return false;
    if(song->duration_secs > filterOptions.maxLength) return false;


    return true;
}

void ResetTable() {
    std::vector<CellData> filteredCells(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());
    songListController->table.child.getStatefulVector(*songListController->table.ctx) = std::move(filteredCells);
    QuestUI::MainThreadScheduler::Schedule([]() {
        songListController->table.update();
    });
}

void SortAndFilterSongs(SortMode sort, std::string_view const search, bool resetTable)
{
    if(songListController != nullptr && songListController->tablePtr)
    {
        songListController->tablePtr->tableView->ClearSelection();
        songListController->tablePtr->tableView->ClearHighlights();
    }
    prevSort = sort;
    prevSearch = search;
    // std::thread([&]{

    bool isRankedSort = sort == SortMode::Most_Stars || sort == SortMode::Least_Stars ||  sort == SortMode::Latest_Ranked;

    //filteredSongList = songList;
    DataHolder::filteredSongList.clear();
    for(auto song : DataHolder::songList)
    {
        if(deezContainsDat(song, split(search, " ")) && (!isRankedSort || rankedFilterFunction(song)) && MeetsFilter(song))
        {
            DataHolder::filteredSongList.emplace_back(song);
        }
    }
    std::stable_sort(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end(),
                     sortFunctionMap.at(sort)
    );

    if(resetTable)
        ResetTable();
    getLogger().info("table reset");
    //}).detach();
}

void Sort(bool resetTable)
{
    SortAndFilterSongs(prevSort, prevSearch, resetTable);
}

inline auto SortDropdownContainer() {
    std::array<std::string, 7> sortModes(
            {"Newest", "Oldest", "Latest Ranked", "Most Stars", "Least Stars", "Best rated", "Worst rated"});

    SongListDropDown<std::tuple_size_v<decltype(sortModes)>> sortDropdown("", "Newest", [sortModes](
            auto &, std::string const &input,
            UnityEngine::Transform *, RenderContext &ctx) {
        getLogger().debug("DropDown! %s", input.c_str());
        auto itr = std::find(sortModes.begin(), sortModes.end(), input);
        SortAndFilterSongs((SortMode)std::distance(sortModes.begin(), itr), prevSearch, true);
    }, sortModes);

    detail::VerticalLayoutGroup layout(sortDropdown);
    layout.padding = std::array<float, 4>{1,1,1,1};

    ModifyLayoutElement layoutElement(layout);
    layoutElement.preferredWidth = 44;

    return layoutElement;
}

inline void onRenderTable(ViewControllers::SongListViewController* view, decltype(ViewControllers::SongListViewController::TableType::child)::RenderState& tableState) {
    view->tablePtr = tableState.dataSource;
    getLogger().info("Rendering table! %p", view->tablePtr);

    if (view->table.renderedAllowed.getData()) {
        CRASH_UNLESS(tableState.dataSource);
        CRASH_UNLESS(view->tablePtr);
        CRASH_UNLESS(view->tablePtr->tableView);

        //Make Lists
        auto click = std::function([view](HMUI::TableView *tableView, int row) {
            getLogger().info("selected %i", row);
            // Get song list actually inside the table
            auto const &songList = *reinterpret_cast<BetterSongSearch::UI::QUCObjectTableData *>(tableView->dataSource)->descriptors;
            view->selectedSongController.child.SetSong(songList[row].song);
        });
        auto yes = il2cpp_utils::MakeDelegate<System::Action_2<HMUI::TableView *, int> *>(
                classof(System::Action_2<HMUI::TableView *, int>*), click);



        getLogger().debug("Adding table event");

        view->tablePtr->tableView->add_didSelectCellWithIdxEvent(yes);

        getLogger().debug("Set sibling");
        view->tablePtr->get_transform()->get_parent()->SetAsFirstSibling();

        // queue for next frame
        QuestUI::MainThreadScheduler::Schedule([]{
            //fix scrolling lol
            // doesn't work - Fern :(
            GlobalNamespace::IVRPlatformHelper* mewhen;
            auto scrolls = UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::ScrollView*>();
            for (int i = 0; i < scrolls.Length(); i++)
            {
                mewhen = scrolls.get(i)->platformHelper;
                if(mewhen != nullptr)
                    break;
            }
            for (int i = 0; i < scrolls.Length(); i++)
            {
                if(scrolls.get(i)->platformHelper == nullptr) scrolls.get(i)->platformHelper = mewhen;
            }
        });
    }
}

inline auto SelectedSongControllerLayout(ViewControllers::SongListViewController* view) {
    auto defaultImage = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>().First([](UnityEngine::Sprite* x) {return x->get_name() == "CustomLevelsPack"; });

    auto& controller = view->selectedSongController;
    controller.child.defaultImage = defaultImage;

    QUC::RefComp selectedSongControllerRefComp(controller);

#pragma region table
    if (view->table.renderedAllowed.getData()) {
        std::vector<CellData> filteredSongs(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());

        view->table.child.initCellDatas = filteredSongs;
    }

    OnRenderCallback tableRender(
            QUC::detail::refComp(view->table),
            [view](auto& self, RenderContext &ctx, RenderContextChildData& data) {
                auto& tableState = ctx.getChildDataOrCreate(self.child.child.key).template getData<decltype(ViewControllers::SongListViewController::TableType::child)::RenderState>();
                onRenderTable(view, tableState);
            }
    );

    ModifyLayoutElement tableContainer(VerticalLayoutGroup(tableRender));
    tableContainer.preferredWidth = 70.0f;

#pragma endregion

    view->loadingIndicatorContainer.emplace(QUC::detail::refComp(view->loadingIndicator));

    return SongListHorizontalLayout(
            tableContainer,
            QUC::detail::refComp(*view->loadingIndicatorContainer),
            selectedSongControllerRefComp
    );
}

custom_types::Helpers::Coroutine checkIfLoaded(ViewControllers::SongListViewController* view) {
    if (view->table.renderedAllowed.getData())
        co_return;

    while (true) {
        view->table.renderedAllowed = DataHolder::loadedSDC;
        view->loadingIndicator.enabled = !DataHolder::loadedSDC;

        co_yield nullptr;

        if (view->table.renderedAllowed.getData()) {
            getLogger().debug("Showing table now");

            std::vector<CellData> filteredCells(DataHolder::filteredSongList.begin(), DataHolder::filteredSongList.end());
            view->table.child.initCellDatas = std::move(filteredCells);

            view->table.update();
            view->loadingIndicatorContainer->update();

            auto& tableState = view->table.ctx->getChildDataOrCreate(view->table.child.key).template getData<decltype(ViewControllers::SongListViewController::TableType::child)::RenderState>();

            onRenderTable(view, tableState);

            co_return;
        }
    }

}

//SongListViewController
void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    songListController = this;

    if (firstActivation) {
        this->ctx = RenderContext(get_transform());
    }

    table.renderedAllowed = DataHolder::loadedSDC;
    loadingIndicator.enabled = !DataHolder::loadedSDC;

    getLogger().debug("Is loading: %s", loadingIndicator.enabled.getData() ? "true" : "false");

    // Periodically check if SDC loaded and update the view
    if (firstActivation && loadingIndicator.enabled.getData()) {
        this->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(checkIfLoaded(this)));
    }

    auto start = std::chrono::high_resolution_clock::now();

    if (firstActivation) {


        auto songListControllerView = SongListVerticalLayoutGroup(
                SongListHorizontalFilterBar(
                        Button("RANDOM", [](Button &button, UnityEngine::Transform *transform, RenderContext &ctx) {
                            int random = rand() % DataHolder::filteredSongList.size();
                            songListController->tablePtr->tableView->ScrollToCellWithIdx(random,
                                                                                         HMUI::TableView::ScrollPositionType::Center,
                                                                                         true);
                        }),
                        StringSetting("Search by Song, Key, Mapper...",
                                      [](StringSetting &, std::string const &input, UnityEngine::Transform *,
                                         RenderContext &ctx) {
                                          getLogger().debug("Input! %s", input.c_str());
                                          std::thread([input]{
                                              SortAndFilterSongs(prevSort, input, true);
                                              /*songListController->table.child.getStatefulVector(songListController->ctx) = std::vector<CellData>(DataHolder::filteredSongList.begin(),DataHolder::filteredSongList.end());
                                              QuestUI::MainThreadScheduler::Schedule([]() {
                                                  songListController->table.update();
                                              });*/
                                          }).detach();
                                      }),
                        SortDropdownContainer()
                ),
                SelectedSongControllerLayout(this)
        );

        auto end = std::chrono::high_resolution_clock::now();
        auto difference = end - start;
        auto millisElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(difference).count();
        getLogger().debug("QUC UI construction for SongList took %lldms", millisElapsed);

        getLogger().debug("Rendering layout");
        detail::renderSingle(songListControllerView, ctx);
        getLogger().debug("Rendered layout");
    }

    if (!firstActivation &&
        DataHolder::loadedSDC && table.renderedAllowed.isRenderDiffModified(*table.ctx) // returns true if it's the state isn't updated
            ) {
        table.update();
        loadingIndicatorContainer->update();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto difference = end - start;
    auto millisElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(difference).count();
    getLogger().debug("UI rendering for SongList took %lldms", millisElapsed);

    getLogger().debug("Finished song list view controller");
}