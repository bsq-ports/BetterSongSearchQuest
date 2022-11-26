#pragma once

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include <fmt/core.h>
#include "paper/shared/logger.hpp"
#include "modloader/shared/modloader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "UnityEngine/GameObject.hpp"

// Define these functions here so that we can easily read configuration and log information from other files
Configuration& getConfig();
Paper::ConstLoggerContext<17UL> getLogger();
Logger& getLoggerOld();

template<typename... TArgs>
constexpr static void fmtLog(Logging::Level lvl, fmt::format_string<TArgs...> str, TArgs&&... args) noexcept {
    getLoggerOld().log(lvl, fmt::format<TArgs...>(str, std::forward<TArgs>(args)...));
}

template<typename Exception = std::runtime_error, typename... TArgs>
inline static void fmtThrowError(fmt::format_string<TArgs...> str, TArgs&&... args) {
    fmtLog<TArgs...>(Logging::ERROR, str, std::forward<TArgs>(args)...);
    throw Exception(fmt::format<TArgs...>(str, std::forward<TArgs>(args)...));
}

#define INFO(...) getLogger().fmtLog<Paper::LogLevel::INF>(__VA_ARGS__)
#define ERROR(...) getLogger().fmtLog<Paper::LogLevel::ERR>(__VA_ARGS__)
#define CRITICAL(...) getLogger().fmtLog<Paper::LogLevel::ERR>(__VA_ARGS__)
#define DEBUG(...) getLogger().fmtLog<Paper::LogLevel::DBG>(__VA_ARGS__)
#define WARNING(...) getLogger().fmtLog<Paper::LogLevel::WRN>(__VA_ARGS__)
