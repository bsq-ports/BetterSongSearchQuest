#pragma once

#include "IHook.hpp"
using namespace BetterSongSearch;

namespace BetterSongSearch::Hooks {
    class DismissFlowCoordinatorHook : IHook {
    public:
        /*
         * ADD YOUR OWN FIELDS
         */
        static bool returnTobss;
        // ---------------------

        void AddHooks() override;
        explicit DismissFlowCoordinatorHook(const std::string& name) : IHook(name);
    };

    bool DismissFlowCoordinatorHook::returnTobss = false;
}