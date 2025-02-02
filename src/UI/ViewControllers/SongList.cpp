#include "UI/ViewControllers/SongList.hpp"

#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/AudioType.hpp"
#include "UnityEngine/Networking/DownloadHandlerAudioClip.hpp"
#include "UnityEngine/Networking/UnityWebRequestMultimedia.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/NoTransitionsButton.hpp"
#include "HMUI/InputFieldViewChangeBinder.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/TableView.hpp"
#include "songcore/shared/SongCore.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/SongPreviewPlayer.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "System/StringComparison.hpp"
#include "System/Nullable_1.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "bsml/shared/BSML.hpp"
#include "fmt/fmt/include/fmt/core.h"
#include "Util/SongUtil.hpp"
#include <iterator>
#include <string>
#include <algorithm>
#include <regex>
#include <future>

#include "PluginConfig.hpp"
#include "assets.hpp"
#include "logging.hpp"
#include "BeatSaverRegionManager.hpp"
#include "Util/BSMLStuff.hpp"
#include "Util/Random.hpp"
#include "UI/FlowCoordinators/BetterSongSearchFlowCoordinator.hpp"

#include "UI/Manager.hpp"
#include "Util/TextUtil.hpp"
#include "Util/Debug.hpp"
#include "Util/CurrentTimeMs.hpp"
#include <limits>
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include "bsml/shared/Helpers/utilities.hpp"
#include "DataHolder.hpp"

#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))


using namespace BetterSongSearch::UI;
using namespace BetterSongSearch::Util;
using namespace BetterSongSearch::UI::Util::BSMLStuff;
using namespace GlobalNamespace;
using namespace SongDetailsCache;
using namespace UnityEngine;

#define SONGDOWNLOADER


SongPreviewPlayer *songPreviewPlayer = nullptr;
LevelCollectionViewController *levelCollectionViewController = nullptr;

DEFINE_TYPE(ViewControllers::SongListController, SongListController);

const std::vector<std::string> CHAR_GROUPING = {"Unknown", "Standard", "OneSaber", "NoArrows", "Lightshow",
                                                "NintyDegree", "ThreeSixtyDegree", "Lawless"};
const std::vector<std::string> CHAR_FILTER_OPTIONS = {"Any", "Custom", "Standard", "One Saber", "No Arrows",
                                                      "90 Degrees", "360 Degrees", "Lightshow", "Lawless"};
const std::vector<std::string> DIFFS = {"Easy", "Normal", "Hard", "Expert", "Expert+"};
const std::vector<std::string> REQUIREMENTS = {"Any", "Noodle Extensions", "Mapping Extensions", "Chroma", "Cinema"};

void ViewControllers::SongListController::_UpdateSearchedSongsList() {
    dataHolder.Search();
}

void ViewControllers::SongListController::UpdateSearchedSongsList() {
    this->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(
        limitedUpdateSearchedSongsList->CallNextFrame()
    ));
}

void ViewControllers::SongListController::PostParse() {
    // Steal search box from the base game
    UnityW<HMUI::InputFieldView> gameSearchBox;
    gameSearchBox = Resources::FindObjectsOfTypeAll<HMUI::InputFieldView *>()->First(
        [](HMUI::InputFieldView *x) {
            return x->get_name() == "SearchInputField";
        }
    );

    if (gameSearchBox) {
        DEBUG("Found search box");
        // Cleanup the old search
        if (searchBox) {
            UnityEngine::Object::DestroyImmediate(searchBox);
        }
        searchBox = Instantiate(gameSearchBox->get_gameObject(), searchBoxContainer->get_transform(), false);
        auto songSearchInput = searchBox->GetComponent<HMUI::InputFieldView *>();
        songSearchPlaceholder = searchBox->get_transform()->Find(
                "PlaceholderText")->GetComponent<HMUI::CurvedTextMeshPro *>();
        songSearchPlaceholder->set_text("Search by Song, Key, Mapper..");
        songSearchInput->____keyboardPositionOffset = Vector3(-15, -36, 0);

        std::function<void(UnityW<HMUI::InputFieldView> view)> onValueChanged = [this](
                UnityW<HMUI::InputFieldView> view) {
            DEBUG("Input is: {}", (std::string) view->get_text());
            this->SortAndFilterSongs(dataHolder.sort, (std::string) view->get_text(), true);
        };

        songSearchInput->get_onValueChanged()->AddListener(BSML::MakeUnityAction(onValueChanged));
    }

    // Get the default cover image
    defaultImage = BSML::Utilities::LoadSpriteRaw(Assets::CustomLevelsCover_png);
    // Set default cover image
    coverImage->set_sprite(defaultImage);

    // Get song preview player 
    songPreviewPlayer = UnityEngine::Resources::FindObjectsOfTypeAll<SongPreviewPlayer *>()->FirstOrDefault();
    levelCollectionViewController = UnityEngine::Resources::FindObjectsOfTypeAll<LevelCollectionViewController *>()->FirstOrDefault();

    // BSML has a bug that stops getting the correct platform helper and on game reset it dies and the scrollhelper stays invalid and scroll doesn't work
    auto platformHelper = Resources::FindObjectsOfTypeAll<LevelCollectionTableView *>()->First()->GetComponentInChildren<HMUI::ScrollView *>()->____platformHelper;
    if (platformHelper == nullptr) {
    } else {
        for (auto x: this->GetComponentsInChildren<HMUI::ScrollView *>()) {
            x->____platformHelper = platformHelper;
        }
    }

    // Make the sort dropdown bigger
    auto c = std::min(9, this->get_sortModeSelections()->____size);
    sortDropdown->dropdown->____numberOfVisibleCells = c;
    sortDropdown->dropdown->ReloadData();
    auto m = sortDropdown->dropdown->____modalView;
    m->get_transform().cast<RectTransform>()->set_pivot(UnityEngine::Vector2(0.5f, 0.83f + (c * 0.011f)));

    if (this->songList) {
        songList->tableView->SetDataSource(reinterpret_cast<HMUI::TableView::IDataSource *>(this), false);
    }
}

void ViewControllers::SongListController::DidActivate(bool firstActivation, bool addedToHeirarchy,
                                                      bool screenSystemDisabling) {
    // Init everything before we call events
    if (firstActivation) {
        DEBUG("SongListController first activation");
        if (dataHolder.playerDataModel == nullptr) {
            dataHolder.playerDataModel = UnityEngine::GameObject::FindObjectOfType<GlobalNamespace::PlayerDataModel *>();
        };

        // Get coordinators
        soloFreePlayFlowCoordinator = UnityEngine::Object::FindObjectOfType<SoloFreePlayFlowCoordinator *>();
        multiplayerLevelSelectionFlowCoordinator = UnityEngine::Object::FindObjectOfType<MultiplayerLevelSelectionFlowCoordinator *>();

        // Get regional beat saver urls
        BeatSaverRegionManager::RegionLookup();

        limitedUpdateSearchedSongsList = new BetterSongSearch::Util::RatelimitCoroutine([this]() {
            DEBUG("UpdateSearchedSongsList limited called");
            this->_UpdateSearchedSongsList();
        }, 0.1f);

        IsSearching = false;
        INFO("Song list contoller activated");

        // Get sort setting from config
        auto sortMode = getPluginConfig().SortMode.GetValue();
        if (sortMode < get_sortModeSelections()->get_Count()) {
            selectedSortMode = get_sortModeSelections()->get_Item(sortMode);
            dataHolder.sort = (FilterTypes::SortMode) sortMode;
        }

        BSML::parse_and_construct(Assets::SongList_bsml, this->get_transform(), this);

        multiDlModal = this->get_gameObject()->AddComponent<UI::Modals::MultiDL *>();
        settingsModal = this->get_gameObject()->AddComponent<UI::Modals::Settings *>();
        uploadDetailsModal = this->get_gameObject()->AddComponent<UI::Modals::UploadDetails *>();

        // If loaded, refresh
        if (dataHolder.loaded) {
            DEBUG("Loaded is true");
            // Initial search
            dataHolder.forceReload = true;
            fcInstance->SongListController->SortAndFilterSongs(dataHolder.sort, dataHolder.search, true);
            fcInstance->FilterViewController->datasetInfoLabel->set_text(
                    fmt::format("{} songs in dataset ", dataHolder.songDetails->songs.size()));
        } else {
            this->DownloadSongList();
        }

        #ifdef HotReload
            fileWatcher->filePath = "/sdcard/bsml/BetterSongSearch/SongList.bsml";
            fileWatcher->checkInterval = 0.5f;
        #endif
    }
    // End first activation

    fromBSS = false;

    // Retry if failed to dl
    this->RetryDownloadSongList();

    // If needs a refresh, start a new search when activated
    if (dataHolder.needsRefresh) {
        // Initial search
        dataHolder.needsRefresh = false; // Clear the flag
        dataHolder.forceReload = true;
        fcInstance->SongListController->SortAndFilterSongs(dataHolder.sort, dataHolder.search, true);
        fcInstance->FilterViewController->datasetInfoLabel->set_text(
                fmt::format("{} songs in dataset ", dataHolder.songDetails->songs.size()));
    }

    // Restore search songs count
    if (dataHolder.loaded && !dataHolder.failed && songSearchPlaceholder) {
        if (dataHolder.filteredSongList.size() < dataHolder.songDetails->songs.size()) {
            songSearchPlaceholder->set_text(fmt::format("Search {} songs", dataHolder.filteredSongList.size()));
        } else {
            songSearchPlaceholder->set_text("Search by Song, Key, Mapper..");
        }
    }
}

void ViewControllers::SongListController::SelectSongByHash(std::string hash) {
    if (dataHolder.songDetails == nullptr) {
        DEBUG("Song details is not loaded yet");
        return;
    }

    DEBUG("Song hash: {}", hash);
    auto &song = dataHolder.songDetails->songs.FindByHash(hash);
    if (song == SongDetailsCache::Song::none) {
        DEBUG("Uh oh, you somehow downloaded a song that was only a figment of your imagination");
        return;
    }

    SetSelectedSong(&song);
}


void ViewControllers::SongListController::SelectSong(UnityW<HMUI::TableView> table, int id) {
    if (!table)
        return;
    DEBUG("Cell clicked {}", id);
    if (dataHolder.displayedSongList.size() <= id) {
        // Return if the id is invalid
        return;
    }
    auto song = dataHolder.displayedSongList[id];
    DEBUG("Selecting song {}", id);
    this->SetSelectedSong(song);

}

float ViewControllers::SongListController::CellSize() {
    // TODO: Different font sizes
    bool smallFont = getPluginConfig().SmallerFontSize.GetValue();
    return smallFont ? 11.66f : 14.0f;
}

void ViewControllers::SongListController::ResetTable() {

    if (songListTable() != nullptr) {
        DEBUG("Songs size: {}", dataHolder.displayedSongList.size());
        DEBUG("TABLE RESET");
        songListTable()->ReloadData();
        songListTable()->ScrollToCellWithIdx(0, HMUI::TableView::ScrollPositionType::Beginning, false);
    }
}

int ViewControllers::SongListController::NumberOfCells() {
    return dataHolder.displayedSongList.size();
}

void ViewControllers::SongListController::ctor() {
    DEBUG("SongListController ctor");
    INVOKE_CTOR();
    selectedSortMode = StringW("Newest");

    // Sub to events
    dataHolder.loadingFinished += {&ViewControllers::SongListController::SongDataDone, this};
    dataHolder.loadingFailed += {&ViewControllers::SongListController::SongDataError, this};
    dataHolder.searchEnded += {&ViewControllers::SongListController::SearchDone, this};
}

void ViewControllers::SongListController::OnDestroy() {
    DEBUG("SongListController onDestroy");
    // Unsub from events
    dataHolder.loadingFinished -= {&ViewControllers::SongListController::SongDataDone, this};
    dataHolder.loadingFailed -= {&ViewControllers::SongListController::SongDataError, this};
    dataHolder.searchEnded -= {&ViewControllers::SongListController::SearchDone, this};
}

void ViewControllers::SongListController::SelectRandom() {
    DEBUG("SelectRandom");
    auto cellsNumber = this->NumberOfCells();
    if (cellsNumber < 2) {
        return;
    }
    auto id = BetterSongSearch::Util::random(0, cellsNumber - 1);
    songListTable()->SelectCellWithIdx(id, true);
    songListTable()->ScrollToCellWithIdx(id, HMUI::TableView::ScrollPositionType::Beginning, false);
};

void ViewControllers::SongListController::ShowMoreModal() {
    this->moreModal->Show(false, false, nullptr);
    DEBUG("ShowMoreModal");
};

void ViewControllers::SongListController::HideMoreModal() {
    this->moreModal->Hide(false, nullptr);
    DEBUG("HideMoreModal");
};

void ViewControllers::SongListController::ShowCloseConfirmation() {
    this->downloadCancelConfirmModal->Show();
    DEBUG("ShowCloseConfirmation");
};

void ViewControllers::SongListController::ForcedUIClose() {
    fcInstance->ConfirmCancelCallback(true);
    this->downloadCancelConfirmModal->Hide();
};

void ViewControllers::SongListController::ForcedUICloseCancel() {
    fcInstance->ConfirmCancelCallback(false);
    this->downloadCancelConfirmModal->Hide();
};

custom_types::Helpers::Coroutine ViewControllers::SongListController::UpdateDataAndFiltersCoro() {
    // Wait
    co_yield reinterpret_cast<System::Collections::IEnumerator *>(UnityEngine::WaitForSeconds::New_ctor(0.1f));

    // WARNING: There is a bug with bsml update, it runs before the value is changed for some reason
    bool filtersChanged = false;


    FilterTypes::SortMode sort = dataHolder.sort;
    if (selectedSortMode != nullptr) {
        int index = get_sortModeSelections()->IndexOf(reinterpret_cast<System::String *> (selectedSortMode.convert()));
        if (index < 0) {}
        else {
            if (index != getPluginConfig().SortMode.GetValue()) {
                filtersChanged = true;
                getPluginConfig().SortMode.SetValue(index);
                sort = (FilterTypes::SortMode) index;
            }
        }
    }

    if (filtersChanged) {
        DEBUG("Sort changed");
        this->SortAndFilterSongs(sort, dataHolder.search, true);
    }
}

void ViewControllers::SongListController::UpdateDataAndFilters() {
    coro(UpdateDataAndFiltersCoro());
    DEBUG("UpdateDataAndFilters");
}


void ViewControllers::SongListController::ShowPlaylistCreation() {
    // Hide modal cause bsml does not support automagic hiding of it
    this->moreModal->Hide(false, nullptr);
    DEBUG("ShowPlaylistCreation");
};

void ViewControllers::SongListController::ShowSettings() {

    this->settingsModal->OpenModal();
    // Hide modal cause bsml does not support automagic hiding of it
    this->moreModal->Hide(false, nullptr);
    DEBUG("ShowSettings");
}

// Song buttons
void ViewControllers::SongListController::Download() {
#ifdef SONGDOWNLOADER

    auto songData = this->currentSong;

    if (songData != nullptr) {
        fcInstance->DownloadHistoryViewController->TryAddDownload(songData);
        downloadButton->set_interactable(false);
    } else {
        WARNING("Current song is null, doing nothing");
    }
#endif
}

void GetByURLAsync(std::string url, std::function<void(bool success, std::vector<uint8_t>)> finished) {
    std::thread([url, finished] {
        auto response = WebUtils::GetAsync<WebUtils::DataResponse>(
            WebUtils::URLOptions(url)
        );

        response.wait();
        
        auto responseValue = response.get();

        bool success = responseValue.IsSuccessful();
        if (!success) {
            DEBUG("Failed to get response for cover image");
            finished(false, {});
            return;
        }
        if (!responseValue.responseData.has_value()) {
            DEBUG("No value in responseData for cover image");
            finished(false, {});
            return;
        }

        auto data = responseValue.responseData.value();

        finished(true, data);
    }).detach();
}

custom_types::Helpers::Coroutine GetPreview(std::string url, std::function<void(UnityEngine::AudioClip *)> finished) {
    auto webRequest = UnityEngine::Networking::UnityWebRequestMultimedia::GetAudioClip(url,
                                                                                       UnityEngine::AudioType::MPEG);
    co_yield reinterpret_cast<System::Collections::IEnumerator *>(CRASH_UNLESS(webRequest->SendWebRequest()));
    if (webRequest->GetError() != UnityEngine::Networking::UnityWebRequest::UnityWebRequestError::OK) {
        INFO("Network error");
        finished(nullptr);
        co_return;
    } else {
        while (!webRequest->get_isDone());
        INFO("Download complete");
        UnityEngine::AudioClip *clip = UnityEngine::Networking::DownloadHandlerAudioClip::GetContent(webRequest);
        DEBUG("Clip size: {}", pretty_bytes(webRequest->get_downloadedBytes()));
        finished(clip);
    }
    co_return;
}

void ViewControllers::SongListController::EnterSolo(GlobalNamespace::BeatmapLevel *level) {
    if (level == nullptr) {
        ERROR("Level is null, refusing to continue");
        return;
    }
    fcInstance->Close(true, false);

    auto customLevelsPack = SongCore::API::Loading::GetCustomLevelPack();
    if (customLevelsPack == nullptr) {
        ERROR("CustomLevelsPack is null, refusing to continue");
        return;
    }
    if (customLevelsPack->___beatmapLevels->get_Length() == 0) {
        ERROR("CustomLevelsPack has no levels, refusing to continue");
        return;
    }

    auto category = SelectLevelCategoryViewController::LevelCategory(
            SelectLevelCategoryViewController::LevelCategory::All);

    // static_assert(sizeof (System::Nullable_1<SelectLevelCategoryViewController::LevelCategory>) == 0x8)
    auto levelCategory = System::Nullable_1<SelectLevelCategoryViewController::LevelCategory>();
    levelCategory.value = category;
    levelCategory.hasValue = true;

    auto state = LevelSelectionFlowCoordinator::State::New_ctor(
            customLevelsPack,
            static_cast<GlobalNamespace::BeatmapLevel *>(level)
    );

    state->___levelCategory = levelCategory;

    multiplayerLevelSelectionFlowCoordinator->LevelSelectionFlowCoordinator::Setup(state);
    soloFreePlayFlowCoordinator->Setup(state);

    manager.GoToSongSelect();

    // For some reason setup does not work for multiplayer so I have to use this method to workaround
    if (
            multiplayerLevelSelectionFlowCoordinator &&
            multiplayerLevelSelectionFlowCoordinator->___levelSelectionNavigationController &&
            multiplayerLevelSelectionFlowCoordinator->___levelSelectionNavigationController->____levelCollectionNavigationController
            ) {
        DEBUG("Selecting level in multiplayer");

        multiplayerLevelSelectionFlowCoordinator->___levelSelectionNavigationController->____levelCollectionNavigationController->SelectLevel(
                static_cast<GlobalNamespace::BeatmapLevel *>(level)
        );
    };
}


void ViewControllers::SongListController::Play() {
    this->PlaySong();
}

void ViewControllers::SongListController::PlaySong(const SongDetailsCache::Song *songToPlay) {
    if (songToPlay == nullptr) {
        songToPlay = currentSong;
        if (currentSong == nullptr) {
            ERROR("Current song is null and songToPlay is null");
            return;
        }
    }

    if (fcInstance->ConfirmCancelOfPending([this, songToPlay]() { PlaySong(songToPlay); }))
        return;

    // Hopefully not leaking any memory
    auto fun = [this, songToPlay]() {
        auto level = SongCore::API::Loading::GetLevelByHash(songToPlay->hash());

        // Fallback for rare cases when the hash is different from the hash in our database (e.g. song got updated)
        if (level == nullptr) {
            // Get song beatsaver id
            std::string songKey = fmt::format("{:X}", songToPlay->mapId());
            songKey = toLower(songKey);
            DEBUG("Looking for level by beatsaver id in path: {}", songKey);
            level = SongCore::API::Loading::GetLevelByFunction(
                    [mapId = songKey](auto level) {
                        auto levelPath = level->get_customLevelPath();
                        return levelPath.find(mapId) != std::string::npos;
                    }
            );
        }

        // If all else fails, cancel
        if (level == nullptr) {
            DEBUG("Hash: {}", songToPlay->hash());
            ERROR("Song somehow is not downloaded and could not find it in our database, pls fix");
            return;
        }

        // If we successfully found the level, we can continue
        BSML::MainThreadScheduler::Schedule(
                [this, level] {
                    fromBSS = true;
                    openToCustom = true;
                    EnterSolo(level);
                });
    };

    if (fcInstance->DownloadHistoryViewController->hasUnloadedDownloads) {
        auto future = SongCore::API::Loading::RefreshSongs(false);
        il2cpp_utils::il2cpp_aware_thread([future, fun] {
            future.wait();
            fun();
        });
    } else {
        fun();
    }
}


void ViewControllers::SongListController::ShowBatchDownload() {
    this->multiDlModal->OpenModal();
    this->HideMoreModal();
}

void ViewControllers::SongListController::ShowSongDetails() {
    if (this->currentSong) {
        uploadDetailsModal->OpenModal(this->currentSong);
    }
}

void ViewControllers::SongListController::UpdateDetails() {
    if (currentSong == nullptr) return;

    // Get old sprite if it exists
    UnityEngine::Sprite *oldSprite = this->coverImage->get_sprite();

    auto song = currentSong;
    auto beatmap = SongCore::API::Loading::GetLevelByHash(std::string(song->hash()));
    bool loaded = beatmap != nullptr;
    bool downloaded = fcInstance->DownloadHistoryViewController->CheckIsDownloaded(std::string(song->hash()));

    float minNPS = 500000, maxNPS = 0;
    float minNJS = 500000, maxNJS = 0;
    for (const auto &diff: *song) {
        float nps = (float) diff.notes / (float) song->songDurationSeconds;
        float njs = diff.njs;
        minNPS = std::min(nps, minNPS);
        maxNPS = std::max(nps, maxNPS);

        minNJS = std::min(njs, minNJS);
        maxNJS = std::max(njs, maxNJS);
    }

    // downloadButton.set.text = "Download";
    SetIsDownloaded(downloaded);
    selectedSongDiffInfo->set_text(
            fmt::format("{:.2f} - {:.2f} NPS \n {:.2f} - {:.2f} NJS", minNPS, maxNPS, minNJS, maxNJS));
    selectedSongName->set_text(song->songName());
    selectedSongAuthor->set_text(song->songAuthorName());

    // This part below is here to not break anything on return
#ifdef SONGDOWNLOADER

    // if beatmap is loaded 
    if (loaded) {
        auto cover = BetterSongSearch::Util::getLocalCoverSync(beatmap);
        if (cover != nullptr) {
            this->coverImage->set_sprite(cover);
        } else {
            this->coverImage->set_sprite(defaultImage);

            // Cleanup old sprite
            if (
                // Don't delete defaultImage
                    oldSprite != defaultImage.ptr() &&
                    this->coverImage->get_sprite().unsafePtr() != oldSprite) {
                if (oldSprite != nullptr) {
                    DEBUG("REMOVING OLD SPRITE");
                    auto texture = oldSprite->get_texture();
                    if (texture != nullptr) {
                        UnityEngine::Object::DestroyImmediate(texture);
                    }
                    UnityEngine::Object::DestroyImmediate(oldSprite);
                }
            }
        }
        coverLoading->set_enabled(false);
    } else {

        std::string newUrl = fmt::format("{}/{}.jpg", BeatSaverRegionManager::coverDownloadUrl, toLower(song->hash()));
        DEBUG("{}", newUrl.c_str());
        coverLoading->set_enabled(true);
        GetByURLAsync(newUrl, [this, song, oldSprite](bool success, std::vector<uint8_t> bytes) {
            BSML::MainThreadScheduler::Schedule([this, bytes, song, success, oldSprite] {
                if (success) {
                    std::vector<uint8_t> data = bytes;
                    DEBUG("Image size: {}", pretty_bytes(bytes.size()));
                    if (song->hash() != this->currentSong->hash()) return;
                    Array<uint8_t> *spriteArray = il2cpp_utils::vectorToArray(data);
                    this->coverImage->set_sprite(BSML::Lite::ArrayToSprite(spriteArray));
                } else {
                    this->coverImage->set_sprite(defaultImage);
                }
                coverLoading->set_enabled(false);

                // Cleanup old sprite
                if (
                    // Don't delete old image if it's a default image
                        oldSprite != defaultImage.ptr() &&
                        this->coverImage->get_sprite().unsafePtr() != oldSprite) {
                    if (oldSprite != nullptr) {
                        auto texture = oldSprite->get_texture();
                        if (texture != nullptr) {
                            UnityEngine::Object::DestroyImmediate(texture);
                        }
                        UnityEngine::Object::DestroyImmediate(oldSprite);
                    }
                }
            });
        });
    }

    // If the song is loaded then get it from local sources
    if (loaded) {
        if (!levelCollectionViewController)
            return;

        levelCollectionViewController->SongPlayerCrossfadeToLevelAsync(beatmap,
                                                                       System::Threading::CancellationToken::get_None());
    } else {
        if (!getPluginConfig().LoadSongPreviews.GetValue()) {
        } else {
            if (!songPreviewPlayer)
                return;

            auto ssp = songPreviewPlayer;

            std::string newUrl = fmt::format("{}/{}.mp3", BeatSaverRegionManager::coverDownloadUrl,
                                             toLower(song->hash()));

            coro(GetPreview(
                    newUrl,
                    [ssp](UnityEngine::AudioClip *clip) {
                        if (clip == nullptr) {
                            return;
                        }
                        ssp->CrossfadeTo(clip, -5, 0, clip->get_length(), nullptr);
                    }
            ));
        }
    }

#endif
}

void ViewControllers::SongListController::FilterByUploader() {
    if (!this->currentSong) {
        return;
    }

    fcInstance->FilterViewController->uploadersString = this->currentSong->uploaderName();
    SetStringSettingValue(fcInstance->FilterViewController->uploadersStringControl,
                          (std::string) this->currentSong->uploaderName());
    fcInstance->FilterViewController->UpdateFilterSettings();
    DEBUG("FilterByUploader");
}

// BSML::CustomCellInfo
HMUI::TableCell *ViewControllers::SongListController::CellForIdx(HMUI::TableView *tableView, int idx) {
    return ViewControllers::SongListTableData::GetCell(tableView)->PopulateWithSongData(
            dataHolder.displayedSongList[idx]);
}

void ViewControllers::SongListController::UpdateSearch() {

}

void ViewControllers::SongListController::SortAndFilterSongs(FilterTypes::SortMode sort, std::string_view const search, bool resetTable) {
    // Skip if not active 
    if (!get_isActiveAndEnabled()) return;

    dataHolder.sort = sort;
    dataHolder.search = search;

    this->UpdateSearchedSongsList();
}

void ViewControllers::SongListController::SetSelectedSong(const SongDetailsCache::Song *song) {
    // TODO: Fill all fields, download image, activate buttons
    if (currentSong != nullptr && currentSong->hash() == song->hash()) {
        return;
    }
    currentSong = song;

    DEBUG("Updating details");
    this->UpdateDetails();
}

void ViewControllers::SongListController::SetIsDownloaded(bool isDownloaded, bool downloadable) {
    playButton->get_gameObject()->set_active(isDownloaded);
    playButton->set_interactable(isDownloaded);
    downloadButton->get_gameObject()->set_active(!isDownloaded);
    downloadButton->set_interactable(!isDownloaded);
    infoButton->set_interactable(true);
}


void ViewControllers::SongListController::DownloadSongList() {
    fcInstance->FilterViewController->datasetInfoLabel->set_text("Loading dataset...");
    dataHolder.DownloadSongList();
}

void ViewControllers::SongListController::RetryDownloadSongList() {
    if (dataHolder.failed && !dataHolder.loading && !dataHolder.loaded) {
        this->DownloadSongList();
    }
}

void ViewControllers::SongListController::SearchDone() {
    auto isMainThread = BSML::MainThreadScheduler::CurrentThreadIsMainThread();
    DEBUG("SearchDone, isMainThread: {}", isMainThread);
    DEBUG("Search done in songlist at {}", fmt::ptr(&dataHolder));

    if (!isMainThread) {
        ERROR("Calling SearchDone not on the main thread is not allowed, returning");
        return;
    }
    
    DEBUG("Displaying {} songs", dataHolder.displayedSongList.size());
    if (!songListTable()) {
        // TODO: Actually understand why songListTable isn't available on soft refresh
        WARNING("SongListTable is null, might be a soft refresh, returning as we don't need to reset anything on soft refresh");
        return;
    }

    long long before = 0;
    before = CurrentTimeMs();
    this->ResetTable();
    INFO("table reset in {} ms", CurrentTimeMs() - before);

    if (songSearchPlaceholder) {
        if (dataHolder.filteredSongList.size() == dataHolder.songDetails->songs.size()) {
            songSearchPlaceholder->set_text("Search by Song, Key, Mapper..");
        } else {
            songSearchPlaceholder->set_text(fmt::format("Search {} songs", dataHolder.filteredSongList.size()));
        }
    }

    // Reset song list table selection
    if (!currentSong) {
        SelectSong(songListTable(), 0);
    } else {        
        // Always un-select in the list to prevent wrong-selections on resorting, etc.
        songListTable()->ClearSelection();
    }

    this->searchInProgress->get_gameObject()->set_active(false);


    // Run search again if something wants to
    bool currentSortChanged = dataHolder.currentSort != dataHolder.sort;
    bool currentSearchChanged = dataHolder.currentSearch != dataHolder.search;
    bool currentFilterChanged = !dataHolder.filterOptionsCache.IsEqual(dataHolder.filterOptions);

    IsSearching = false;

    // Queue another search at the end of this one
    if (currentSearchChanged || currentSortChanged || currentFilterChanged) {
        DEBUG("Queueing another search");
        DEBUG("Sort: {}, Search: {}, Filter: {}", currentSortChanged, currentSearchChanged, currentFilterChanged);
        this->UpdateSearchedSongsList();
    }
}

// Event receivers
void ViewControllers::SongListController::SongDataDone() {
    BSML::MainThreadScheduler::Schedule([this] {
        if (this->get_isActiveAndEnabled()) {
            dataHolder.needsRefresh = false;
            // TODO: Move into dataholder
            // Initial search
            dataHolder.forceReload = true;
            fcInstance->SongListController->SortAndFilterSongs(dataHolder.sort, dataHolder.search, true);
        }
    });
}

void ViewControllers::SongListController::SongDataError(std::string message) {

}