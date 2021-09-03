#include "main.hpp"

#include "Hooks/DismissFlowCoordinatorHook.hpp"
using namespace BetterSongSearch::Hooks;

#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
using namespace HMUI;

#include "System/Action.hpp"
using namespace System;

#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
using namespace GlobalNamespace;

MAKE_HOOK_MATCH(FlowCoordinator_DismissFlowCoordinator, &FlowCoordinator::DismissFlowCoordinator, void,
    FlowCoordinator* self, FlowCoordinator* flowCoordinator, ViewController::AnimationDirection animationDirection, Action* finishedCallback, bool immediately
) {
    if (!DismissFlowCoordinatorHook::returnTobss)
        return;

    if (!(il2cpp_utils::try_cast<SoloFreePlayFlowCoordinator*>(flowCoordinator))) { // casting will return nullptr if type cannot be casted, imitating "flowCoordinator is SoloFreePayCoordinator" in C#
        DismissFlowCoordinatorHook::returnTobss = false;
        return;
    }

    immediately = true;

    FlowCoordinator_DismissFlowCoordinator(self, flowCoordinator, animationDirection, finishedCallback, immediately);

    if (!DismissFlowCoordinatorHook::returnTobss)
        return;

    DismissFlowCoordinatorHook::returnTobss = false;

    //Manager::ShowFlow(true);
}

void DismissFlowCoordinatorHook::AddHooks() {
    INSTALL_HOOK(getLogger(), Hook_FlowCoordinator_DismissFlowCoordinator);
}

DismissFlowCoordinatorHook::SoloExitHook(const std::string &name) : IHook(name) {}
