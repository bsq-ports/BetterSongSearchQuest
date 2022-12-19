#pragma once

#include "HMUI/ViewController.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "custom-types/shared/macros.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "UI/ViewControllers/SongList.hpp"
#include "UI/ViewControllers/FilterView.hpp"
#include "UI/ViewControllers/DownloadHistory.hpp"

using namespace BetterSongSearch::UI;
#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::FlowCoordinators, BetterSongSearchFlowCoordinator, HMUI::FlowCoordinator,
    DECLARE_INSTANCE_FIELD(ViewControllers::SongListController*, SongListController);
    DECLARE_INSTANCE_FIELD(ViewControllers::FilterViewController*, FilterViewController);
    DECLARE_INSTANCE_FIELD(ViewControllers::DownloadHistoryViewController*, DownloadHistoryViewController);

    DECLARE_INSTANCE_METHOD(void, Awake);

    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::FlowCoordinator::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD(void, BackButtonWasPressed, GET_FIND_METHOD(&HMUI::FlowCoordinator::BackButtonWasPressed), HMUI::ViewController* topViewController);
)

inline BetterSongSearch::UI::FlowCoordinators::BetterSongSearchFlowCoordinator* fcInstance;