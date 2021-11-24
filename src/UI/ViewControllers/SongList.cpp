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
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Events/UnityAction.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "HMUI/ImageView.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"
#include "HMUI/VerticalScrollIndicator.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "sombrero/shared/Vector2Utils.hpp"
#include "songloader/shared/API.hpp"
#include "System/StringComparison.hpp"
#include "System/Action_2.hpp"
#include <iomanip>
#include <sstream>

#include <time.h>
#include <chrono>
#include <string>
#include <algorithm>
#include <functional>

DEFINE_TYPE(BetterSongSearch::UI::ViewControllers, SongListViewController);
DEFINE_TYPE(CustomComponents, CustomCellListTableData);
DEFINE_TYPE(CustomComponents, SongListCellTableCell);

std::vector<const SDC_wrapper::BeatStarSong*> downloadedSongList;
std::vector<const SDC_wrapper::BeatStarSong*> songList;
std::vector<const SDC_wrapper::BeatStarSong*> filteredSongList;

//SongListCellTableCell
void CustomComponents::SongListCellTableCell::ctor()
{
}

void CustomComponents::SongListCellTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType)
{
    RefreshVisuals();
}

void CustomComponents::SongListCellTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType)
{
    RefreshVisuals();
}

void CustomComponents::SongListCellTableCell::RefreshVisuals()
{
    bool isSelected = get_selected();
    bool isHighlighted = get_highlighted(); 

    bg->background->set_color(UnityEngine::Color(0, 0, 0, isSelected ? 0.9f : isHighlighted ? 0.6f : 0.45f));
}

std::string ShortMapDiffNames(std::string_view input) {
    if(input == "Easy")
        return "E";
    if(input == "Normal")
        return "N";
    if(input == "Hard")
        return "H";
    if(input == "Expert")
        return "Ex";
    if(input == "ExpertPlus")
        return "E+";
    return nullptr;
}

void CustomComponents::SongListCellTableCell::RefreshData(const SDC_wrapper::BeatStarSong* data)
{
    auto name = std::string(data->GetName());
    auto author = std::string(data->GetSongAuthor());
    auto combined = author + " - " + name;
    //if(combined.length() > 40)
    //{
    //    combined.resize(std::min(27, (int)combined.length()));
    //    combined += "...";
    //}
    auto ranked = data->GetMaxStarValue() > 0;

    std::vector<std::u16string> difficulties;
    auto difficultyData = data->GetDifficultyVector();

    for(int i = 0; i < difficultyData.size(); i++) {
        if(ranked) {
            if(difficultyData[i]->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Lightshow()) {
                difficulties.push_back(to_utf16(string_format("<color=white>%s</color>", std::string(ShortMapDiffNames(difficultyData[i]->GetName())).c_str())));
            }
            else
                difficulties.push_back(to_utf16(string_format("<color=white>%s</color> <color=#ffa500>%.1f</color>", std::string(difficultyData[i]->GetName()).c_str(),difficultyData[i]->stars)));
        }
        else {
            if(difficultyData[i]->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Lightshow()) {
                difficulties.push_back(to_utf16(string_format("<color=white>%s</color>", std::string(ShortMapDiffNames(difficultyData[i]->GetName())).c_str())));
            }
            else
                difficulties.push_back(to_utf16(string_format("<color=white>%s</color>", std::string(difficultyData[i]->GetName()).c_str())));
        }
    }

    songText->set_text(il2cpp_utils::newcsstr(combined));

    auto it = std::find(downloadedSongList.begin(), downloadedSongList.end(), data);
    bool downloaded = it != downloadedSongList.end();
    songText->set_color( ranked ? UnityEngine::Color(1,0.647f,0, 1) : (downloaded ? UnityEngine::Color(0.53f, 0.53f, 0.53f, 1.0f) : UnityEngine::Color::get_white()));
    mapperText->set_text(il2cpp_utils::newcsstr(data->GetAuthor()));

    char date[100];
    struct tm *t = gmtime(reinterpret_cast<const time_t*>(&data->uploaded_unix_time));
    strftime(date, sizeof(date), "%e. %b %G", t);
    uploadDateText->set_text(il2cpp_utils::newcsstr(std::string(date)));

    int dur = data->duration_secs;
    std::chrono::seconds sec(dur);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(sec).count();
    dur = dur % 60;
    std::string secondsStr = std::to_string(dur);
    if(secondsStr.length() < 2)
    {
        secondsStr += "0";
    }
    std::string displayTime = std::to_string(minutes) + ":" + secondsStr;
    ratingText->set_text(il2cpp_utils::newcsstr("Length: " + displayTime + " Upvotes: " + std::to_string(data->upvotes) + " Downvotes: " + std::to_string(data->downvotes)));

    diffs->set_texts(difficulties);
    diffs->CellForCellNumber(0)->SetSelected(false, HMUI::SelectableCell::TransitionType::Instant, nullptr, false);
}
//CustomCellListTableData
HMUI::TableCell* CustomComponents::CustomCellListTableData::CellForIdx(HMUI::TableView* tableView, int idx)
{
    static auto SongListCustomCellListCell_cs = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("SongListCellTableCell");
    static auto SongListCustomTableCell_cs = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("SongListCustomTableCell");

    CustomComponents::SongListCellTableCell* tableCell = reinterpret_cast<CustomComponents::SongListCellTableCell*>(tableView->DequeueReusableCellForIdentifier(SongListCustomCellListCell_cs));
    if(!tableCell)
    {
        tableCell = UnityEngine::GameObject::New_ctor()->AddComponent<CustomComponents::SongListCellTableCell*>();
        tableCell->set_reuseIdentifier(SongListCustomCellListCell_cs);
        tableCell->set_name(SongListCustomTableCell_cs);
        auto fitter = tableCell->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        fitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);

        tableCell->get_gameObject()->AddComponent<HMUI::Touchable*>();
        tableCell->set_interactable(true);

        auto verticalLayoutGroup = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(tableCell->get_transform());
        tableCell->bg = verticalLayoutGroup->get_gameObject()->AddComponent<QuestUI::Backgroundable*>();
        tableCell->bg->ApplyBackgroundWithAlpha(il2cpp_utils::newcsstr("round-rect-panel"), 0.6f);
        verticalLayoutGroup->set_padding(UnityEngine::RectOffset::New_ctor(1, 1, 1, 2));
        auto fitter2 = verticalLayoutGroup->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        fitter2->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);

        auto layout = tableCell->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layout->set_preferredHeight(11.75f);
        layout->set_preferredWidth(70);

        tableCell->get_transform()->SetParent(tableView->get_transform()->GetChild(0)->GetChild(0), false);

        auto topHoriz = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());
        topHoriz->set_childForceExpandWidth(true);

        auto midHoriz = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());

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

        tableCell->diffs = QuestUI::BeatSaberUI::CreateTextSegmentedControl(bottomHoriz->get_transform());
        tableCell->diffs->fontSize = 2;
        tableCell->diffs->padding = 1.5;
        tableCell->diffs->overrideCellSize = true;
        tableCell->diffs->firstCellPrefab = Instantiate(tableCell->diffs->firstCellPrefab);
        tableCell->diffs->lastCellPrefab = Instantiate(tableCell->diffs->lastCellPrefab);
        tableCell->diffs->middleCellPrefab = Instantiate(tableCell->diffs->middleCellPrefab);
        tableCell->diffs->singleCellPrefab = Instantiate(tableCell->diffs->singleCellPrefab);

        Destroy(tableCell->diffs->firstCellPrefab->GetComponentInChildren<HMUI::Touchable*>());
        Destroy(tableCell->diffs->lastCellPrefab->GetComponentInChildren<HMUI::Touchable*>());
        Destroy(tableCell->diffs->middleCellPrefab->GetComponentInChildren<HMUI::Touchable*>());
        Destroy(tableCell->diffs->singleCellPrefab->GetComponentInChildren<HMUI::Touchable*>());

        tableCell->diffs->singleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
        tableCell->diffs->lastCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
        tableCell->diffs->firstCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
        tableCell->diffs->middleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
        tableCell->diffs->firstCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
        tableCell->diffs->lastCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
        tableCell->diffs->middleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
        tableCell->diffs->singleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
        tableCell->diffs->hideCellBackground = true;
        //tableCell->diffs->set_texts(std::vector<std::u16string>{u"Ex", u"Lmao"});
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

std::vector<std::string> split(std::string buffer, const std::string delimeter = " ") {
    std::vector<std::string> ret{};
    std::decay_t<decltype(std::string::npos)> pos{};
    while ((pos = buffer.find(delimeter)) != std::string::npos) {
        const auto match = buffer.substr(0, pos);
        if (!match.empty()) ret.push_back(match);
        buffer = buffer.substr(pos + delimeter.size());
    }
    if (!buffer.empty()) ret.push_back(buffer);
    return ret;
}

std::string toLower(std::string_view str) 
{
    std::string stringy(str);
    for(char &ch : stringy){
        ch = std::tolower(ch);
    }
    return stringy;
}

bool strContain(std::string_view str, std::string_view str2) 
{
    return strstr(std::string(str).c_str(), std::string(str2).c_str());
}

std::string removeSpecialCharacter(std::string s)
{
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

bool deezContainsDat(const SDC_wrapper::BeatStarSong* song, std::vector<std::string> searchTexts)
{
    int words = 0;
    int matches = 0;
    auto songName = removeSpecialCharacter(toLower(song->GetName()));
    auto songSubName = removeSpecialCharacter(toLower(song->GetSubName()));
    auto songAuthorName = removeSpecialCharacter(toLower(song->GetSongAuthor()));
    auto levelAuthorName = removeSpecialCharacter(toLower(song->GetAuthor()));

    for (int i = 0; i < searchTexts.size(); i++)
    {
        words++;
        auto searchTerm = toLower(std::string(searchTexts[i]));
        if (i == searchTexts.size() - 1)
        {
            searchTerm.resize(searchTerm.length()-1);
        }

        if (strContain(songName, searchTerm) || 
            strContain(songSubName, searchTerm) ||
            strContain(songAuthorName, searchTerm) ||
            strContain(levelAuthorName, searchTerm))
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
    auto it = std::find(downloadedSongList.begin(), downloadedSongList.end(), song);
    bool downloaded = it != downloadedSongList.end();
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

    bool foundValidDiff = false;
    bool foundValidChar = false;

    for(auto diff : song->GetDifficultyVector())
    {
        float nps = (float)diff->notes / (float)song->duration_secs;
        float njs = diff->njs;
        minNPS = std::min(nps, minNPS);
        maxNPS = std::max(nps, maxNPS);

        minNJS = std::min(njs, minNJS);
        maxNJS = std::max(njs, maxNJS);
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


    return true;
}

void SortAndFilterSongs(int sort, std::string search)
{
    if(tableData != nullptr)
    {
        tableData->tableView->ClearSelection();
        tableData->tableView->ClearHighlights();
    }
    prevSort = sort;
    prevSearch = search;
    //std::thread([&]{
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
        filteredSongList.clear();
        for(auto song : songList)
        {
            if(deezContainsDat(song, split(search, " ")) && sortFilterFuncs[sort](song) && MeetsFilter(song))
            {
                filteredSongList.push_back(song);
            }
        }
        std::sort(filteredSongList.begin(), filteredSongList.end(), 
            sortFuncs[sort]
        );
    //}).detach();
}

void Sort()
{
    SortAndFilterSongs(prevSort, prevSearch);
    tableData->data = filteredSongList;
    tableData->tableView->ReloadData();
}

//SongListViewController
void ViewControllers::SongListViewController::DidActivate(bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling) {
    songListController = this;
    if (!firstActivation) return;

    static ViewComponent* view;

    if (view) {
        delete view;
        view = nullptr;
    }
    std::vector<std::string> sortModes = {"Newest", "Oldest", "Latest Ranked", "Most Stars", "Least Stars", "Best rated", "Worst rated", "Most Downloads"};
    SongListHorizontalLayout* shitass;
    auto downloadedSongs = RuntimeSongLoader::API::GetLoadedSongs();
    // async ui because this causes lag spike
    auto songListTemp = songList;
    std::thread([this, sortModes, &shitass, downloadedSongs, songListTemp]{
        for(GlobalNamespace::CustomPreviewBeatmapLevel* song : downloadedSongs)
        {
            auto levelID = to_utf8(csstrtostr(song->levelID));
            auto it = find_if(songList.begin(), songList.end(), [levelID](const SDC_wrapper::BeatStarSong* obj) {return levelID.ends_with(obj->GetHash());});

            if (it != songList.end())
            {
                auto index = std::distance(songList.begin(), it);
                downloadedSongList.push_back(songList[index]);
            }
        }
        view = new ViewComponent(this->get_transform(), {
            new SongListVerticalLayoutGroup({
                new SongListHorizontalFilterBar({
                    new Button("RANDOM", [songListTemp](Button* button, UnityEngine::Transform* parentTransform){
                        int random = rand() % songListTemp.size();
                        tableData->tableView->ScrollToCellWithIdx(random, HMUI::TableView::ScrollPositionType::Center, true);
                    }),
                    new Button("MULTI", [](Button* button, UnityEngine::Transform* parentTransform){

                    }),
                    new StringSetting("Search songs" , "", [](StringSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("Input! %s", input.c_str());
                        SortAndFilterSongs(prevSort, input);

                        tableData->data = filteredSongList;
                        tableData->tableView->ReloadData();
                    }),

                    new SongListDropDown("", "Newest", sortModes, [sortModes](DropdownSetting*, const std::string& input, UnityEngine::Transform*){
                        getLogger().debug("DropDown! %s", input.c_str());
                        auto itr = std::find(sortModes.begin(), sortModes.end(), input);
                        SortAndFilterSongs(std::distance(sortModes.begin(), itr), prevSearch);

                        tableData->data = filteredSongList;
                        tableData->tableView->ReloadData();
                    }),
                }),
                (new SongListHorizontalLayout({
                    new VerticalLayoutGroup({
                    })
                }))->with([&shitass](SongListHorizontalLayout* scrollableContainer)
                    {
                        shitass = scrollableContainer;
                    })
            })
        });
        QuestUI::MainThreadScheduler::Schedule([this, shitass]{
            view->render();

            auto selectedSongView = shitass->getTransform()->GetChild(0);
            selectedSongView->get_gameObject()->AddComponent<QuestUI::Backgroundable*>()->ApplyBackground(il2cpp_utils::newcsstr("round-rect-panel"));
            auto layout = selectedSongView->get_gameObject()->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>();
            layout->set_childForceExpandHeight(false);
            layout->set_padding(UnityEngine::RectOffset::New_ctor(2,2,2,2));
            selectedSongView->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(40);
            auto controller = selectedSongView->get_gameObject()->AddComponent<SelectedSongController*>();
            selectedSongController = controller;
            controller->defaultImage = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>(), 
            [](UnityEngine::Sprite* x) { 
                return to_utf8(csstrtostr(x->get_name())) == "CustomLevelsPack"; 
            });
            //Meta
            {
                auto metaLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(selectedSongView);
                auto metaFitter = metaLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                metaFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                metaFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

                auto authorText = QuestUI::BeatSaberUI::CreateText(metaLayout->get_transform(), "Author");
                authorText->set_color(UnityEngine::Color(0.8f, 0.8f, 0.8f, 1));
                auto authorFitter = authorText->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                authorFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                authorFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                authorText->set_fontSize(3.2f);
                controller->authorText = authorText;

                auto nameText = QuestUI::BeatSaberUI::CreateText(metaLayout->get_transform(), "Name");
                auto nameFitter = nameText->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                nameFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                nameFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                nameText->set_fontSize(2.7f);
                controller->songNameText = nameText;
            }

            //Cover Image
            {
                auto metaLayout = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(selectedSongView);
                auto metaFitter = metaLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                metaFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                metaFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                auto metaElement = metaLayout->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
                metaElement->set_preferredWidth(28);
                metaElement->set_preferredHeight(28);

                auto cover = QuestUI::BeatSaberUI::CreateImage(metaLayout->get_transform(), controller->defaultImage, UnityEngine::Vector2(0, 0), UnityEngine::Vector2(28, 28));
                auto coverElement = cover->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
                coverElement->set_preferredWidth(28);
                coverElement->set_preferredHeight(28);
                cover->set_preserveAspect(true);
                controller->coverImage = cover;
            }
            
            //Min-Max Diff Info
            {
                auto infoLayout = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(selectedSongView);
                auto infoFitter = infoLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                infoFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                infoFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                controller->infoText = QuestUI::BeatSaberUI::CreateText(infoLayout->get_transform(), "details");
            }

            //Play-Download Buttonss
            {
                auto buttonLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(selectedSongView);
                auto buttonFitter = buttonLayout->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                buttonFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
                buttonFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

                auto downloadButton = QuestUI::BeatSaberUI::CreateUIButton(buttonLayout->get_transform(), "Download", "PlayButton", [controller](){controller->DownloadSong();});
                controller->downloadButton = downloadButton;

                auto playButton = QuestUI::BeatSaberUI::CreateUIButton(buttonLayout->get_transform(), "Play", "PlayButton", [controller](){controller->PlaySong();});
                controller->playButton = playButton;

                auto infoButton = QuestUI::BeatSaberUI::CreateUIButton(buttonLayout->get_transform(), "Song Details", "PracticeButton", nullptr);
            }

            SortAndFilterSongs(0, "");

            //Make Lists
            auto list = QuestUI::BeatSaberUI::CreateScrollableCustomSourceList<CustomComponents::CustomCellListTableData*>(shitass->getTransform(), UnityEngine::Vector2(70, 6 * 11.7f));
            tableData = list;
            list->data = filteredSongList;
            list->tableView->ReloadData();
            auto click = std::function([=](HMUI::TableView* tableView, int row)
            {
                this->selectedSongController->SetSong(filteredSongList[row]);
            });
            auto yes = il2cpp_utils::MakeDelegate<System::Action_2<HMUI::TableView*, int>*>(classof(System::Action_2<HMUI::TableView*, int>*), click);
            list->tableView->add_didSelectCellWithIdxEvent(yes);
            list->get_transform()->get_parent()->SetAsFirstSibling();

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

        });
    }).detach();
}