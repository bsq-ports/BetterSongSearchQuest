#pragma once

#include "main.hpp"

#include <string>
#include <vector>

namespace BetterSongSearch {

    /*
     * DECLARE IHOOK INTERFACE CLASS
     */

    class IHook {
    private:
        static std::vector<IHook> hooks;
        protected:
            std::string name;

    public:
        static bool InstallHooks();

        IHook(const std::string& name);

        virtual void AddHooks();
    };

    /*
     * DEFINE IHOOK INTERFACE CLASS METHODS
     */

    bool IHook::InstallHooks() {
        for (IHook hook : IHook::hooks) {
            hook.AddHooks();
            getLogger().info("Installed %s Hook", hook.name.c_str());
        }
    }

    IHook::IHook(const std::string& name) {
        this->name = name;
    }

    void IHook::AddHooks() {
        getLogger().info("AddHooks method was not overridden for %s Hook", this->name.c_str());
    }
}