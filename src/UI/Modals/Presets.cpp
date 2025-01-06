#include "UI/Modals/Presets.hpp"
#include "HMUI/TableView.hpp"
#include "bsml/shared/BSML.hpp"
#include "System/Tuple_2.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"
#include "logging.hpp"
#include "assets.hpp"
#include "UI/Modals/PresetsTable.hpp"
#include "FilterOptions.hpp"
#include "UI/ViewControllers/SongList.hpp"

using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

DEFINE_TYPE(BetterSongSearch::UI::Modals, Presets);

void Modals::Presets::OnEnable()
{
}

void Modals::Presets::PostParse()
{
    if (this->presetListTableData) {
        INFO("Table exists");
        this->presetListTableData->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);
    }
}

void Modals::Presets::CloseModal()
{
    this->presetsModal->Hide();
}

void Modals::Presets::ctor()
{
    INVOKE_CTOR();
    this->initialized = false;
}

void Modals::Presets::OpenModal()
{
    if (!initialized) {
        BSML::parse_and_construct(Assets::Presets_bsml, this->get_transform(), this);
        initialized = true;
    }
    RefreshPresetsList();
    this->presetsModal->Show();
}

void Modals::Presets::OpenSavePresetModal()
{
    if (!initialized) {
        BSML::parse_and_construct(Assets::Presets_bsml, this->get_transform(), this);
        initialized = true;
    }
    this->savePresetModal->Show();
}

void Modals::Presets::CloseSavePresetModal()
{
    this->savePresetModal->Hide();
}

// Table stuff
HMUI::TableCell *Modals::Presets::CellForIdx(HMUI::TableView *tableView, int idx)
{
    return Modals::PresetsTableCell::GetCell(tableView)->PopulateWithPresetName(presets[idx]);
}

float Modals::Presets::CellSize()
{
    return 8.05f;
}

int Modals::Presets::NumberOfCells()
{
    return presets.size();
}

void Modals::Presets::AddPreset()
{
    try {
        if (newPresetName == "") {
            return;
        }
        DataHolder::filterOptions.SaveToPreset(newPresetName);

        presets.push_back(newPresetName);

    } catch (std::exception &e) {
        ERROR("Failed to save preset: {}", e.what());
    }

    RefreshPresetsList();
    newPresetName = "";

    CloseSavePresetModal();
}

void Modals::Presets::LoadPreset()
{
    if (selectedPreset == "") {
        return;
    }

    auto presetOptional = DataHolder::filterOptions.LoadFromPreset(selectedPreset);

    if (!presetOptional.has_value()) {
        return;
    }

    DataHolder::filterOptions = presetOptional.value();
    DataHolder::filterOptions.SaveToConfig();

    // Update filter settings
    fcInstance->FilterViewController->UpdateLocalState();
    fcInstance->FilterViewController->ForceRefreshUI();

    DEBUG("Filters changed");

    // Update filter options state
    auto slController = fcInstance->SongListController;
    slController->filterChanged = true;
    slController->SortAndFilterSongs(slController->sort, slController->search, true);

    CloseModal();
}

void Modals::Presets::DeletePreset()
{
    if (selectedPreset == "") {
        return;
    }
    DataHolder::filterOptions.DeletePreset(selectedPreset);
    RefreshPresetsList();
    
    if (presetListTableData && presetListTableData->tableView) {
        presetListTableData->tableView->ClearSelection();
    }
    selectedPreset = "";
}

void Modals::Presets::PresetSelected(UnityW<HMUI::TableView> table, int id)
{
    if (id < 0 || id >= presets.size()) {
        return;
    }
    selectedPreset = presets[id];
}

void Modals::Presets::RefreshPresetsList()
{
    auto presetList = FilterProfile::GetPresetList();
    presets.clear();
    
    for (auto &preset : presetList) {
        presets.push_back(preset);
    }

    if (presetListTableData && presetListTableData->tableView) {
        presetListTableData->tableView->ReloadData();
    }
}