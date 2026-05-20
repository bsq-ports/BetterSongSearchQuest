# Better Song Search Quest - Agent Instructions

This repository builds **BetterSongSearch** for **Beat Saber Quest / Android IL2CPP**. It is a Scotland2 mod using beatsaber-hook style hooks, QPM dependency management, BSML UI, config-utils, and bs-cordl generated game bindings.

If these notes conflict with the actual code, trust the code.

## Project Shape

- Entry points live in `src/main.cpp`.
- Exported modloader functions must use `BSS_EXPORT_FUNC` from `include/_config.h`; symbols are hidden by default in `CMakeLists.txt`.
- Current entry points are:
  - `BSS_EXPORT_FUNC void setup(CModInfo& info)`
  - `BSS_EXPORT_FUNC void late_load()`
- `setup` assigns `modInfo`, initializes `getPluginConfig()`, logs setup completion, and starts `dataHolder.Init()` on a detached thread.
- `late_load` initializes IL2CPP, BSML, custom types, installs hooks, and calls `manager.Init()`.

## Dependencies And Target

- QPM metadata is in `qpm.json` and `qpm.shared.json`.
- The mod ID is `BetterSongSearch`; do not change it.
- Major dependencies include `beatsaber-hook`, `bsml`, `custom-types`, `config-utils`, `paper2_scotland2`, `songcore`, `song-details`, `web-utils`, and `beatsaverplusplus`.

## Logging

- Use `include/logging.hpp`.
- Prefer the existing macros: `INFO`, `DEBUG`, `ERROR`, `WARNING`, `CRITICAL`.
- Use the existing `Logger` context when installing hooks:
  - `INSTALL_HOOK(Logger, HookName);`
- Do not add another logging framework.

## Hooks

- Match the existing beatsaber-hook style in `src/main.cpp`.
- Use `MAKE_HOOK_MATCH` for hooks when possible.
- Install hooks from `late_load`.
- Hooks should be tight:
  - validate pointers before dereferencing
  - call the original method when behavior should continue
  - avoid per-frame allocations and expensive searches
  - avoid doing network, filesystem, or long-running work inside hooks
- For Unity objects, prefer `UnityW<T>` or `SafePtrUnity<T>` where the surrounding code does.

## Threading And Unity Safety

- Treat Unity and HMUI objects as main-thread-only unless nearby code proves otherwise.
- Use `BSML::MainThreadScheduler::Schedule(...)` when returning from background work to UI/game objects.
- Coroutines are commonly started with `BSML::SharedCoroutineStarter` and `custom_types::Helpers::CoroutineHelper::New(...)`.
- `DataHolder` owns a lot of search/cache behavior; inspect `include/DataHolder.hpp` and `src/DataHolder.cpp` before changing search flow or song data lifetime.

## UI And Custom Types

- BSML layouts live in `assets/*.bsml`.
- Generated embedded assets are exposed through `include/assets.hpp`.
- View controllers live under `src/UI/ViewControllers` and `include/UI/ViewControllers`.
- Modals live under `src/UI/Modals` and `include/UI/Modals`.
- The main flow coordinator is `BetterSongSearchFlowCoordinator`.
- Follow the existing `DECLARE_CLASS_CODEGEN`, `DECLARE_CLASS_CODEGEN_INTERFACES`, `DECLARE_CLASS_CUSTOM`, and `DEFINE_TYPE` patterns.
- When adding BSML-bound properties or fields, keep the C++ declaration, `.cpp` implementation, and `.bsml` binding names in sync.

## Config

- Config lives in `include/PluginConfig.hpp` through `DECLARE_CONFIG(PluginConfig)`.
- Read/write values with `getPluginConfig().SomeValue.GetValue()` and `.SetValue(...)`.
- Call `getPluginConfig().Save()` when a multi-value update needs to be persisted immediately.
- Filter persistence is centralized in `FilterProfile::LoadFromConfig()` and `FilterProfile::SaveToConfig()` in `src/FilterOptions.cpp`.
- Do not invent a second config system.

## Build And Dev Commands

- Restore dependencies:
  - `qpm restore`
- Build:
  - `qpm s build`
  - or `pwsh ./build.ps1`
- Clean build:
  - `qpm s build -- -clean`
  - or `pwsh ./build.ps1 -clean`
- Release-style build:
  - `pwsh ./build.ps1 -release`
- Build, copy to Quest, restart, and optionally log:
  - `pwsh ./copy.ps1 -log -file`
  - or `qpm s copy -- -log -file`
- Start logcat separately:
  - `pwsh ./start-logging.ps1`
- Build qmod:
  - `qpm s qmod`
  - or `pwsh ./createqmod.ps1 BetterSongSearch`

## Packaging

- `mod.template.json` is the template for mod metadata.
- QPM output is `BetterSongSearch.qmod`.
- The built mod library is `libBetterSongSearch.so`.
- The `.so` is packaged as a late mod file.
- Keep metadata valid and minimal. Do not change the established mod ID.

## Code Style

- Match nearby formatting, include ordering, namespaces, and naming.
- This project uses C++20.
- CMake currently enables RTTI and exceptions; do not fight that, but also do not add exception-heavy or RTTI-heavy designs without a clear local reason.
- Keep headers lean and prefer forward declarations where the repo already does.
- Avoid `std::string` churn in hot paths; follow existing patterns around `StringW`, `std::string`, and cached values.
- Add comments only when they clarify non-obvious game/modloader behavior.

## Do Not

- Do not write PC BSIPA/Mono mod code.
- Do not add analytics, telemetry, or unrelated network behavior.
- Do not patch or modify the APK in code; packaging/install tools handle deployment.
- Do not change build metadata, qmod metadata, or dependency versions unless the task explicitly needs it.
