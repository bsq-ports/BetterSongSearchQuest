# Better Song Search Quest - Agent Instructions

This repository builds **BetterSongSearch** for **Beat Saber Quest / Android IL2CPP**. It is a Scotland2 mod using beatsaber-hook style hooks, QPM dependency management, BSML UI, config-utils, and bs-cordl generated game bindings.

If these notes conflict with the actual code, trust the code.

## Project Shape

- Entry points live in `src/main.cpp`.
- Exported modloader functions must use `BSS_EXPORT_FUNC` from `include/_config.h`; symbols are hidden by default in `CMakeLists.txt`.
- Current entry points are:
  - `BSS_EXPORT_FUNC void setup(CModInfo& info)`
  - `BSS_EXPORT_FUNC void late_load()`
- `setup` assigns `modInfo` and logs setup completion.
- `late_load` initializes BSML and custom types, loads and validates config before starting `dataHolder.Init()` on a detached thread, installs hooks, and calls `manager.Init()`.

## Dependencies And Target

- QPM metadata is in `qpm.json` and `qpm.shared.json`.
- The mod ID is `BetterSongSearch`; do not change it.
- Major dependencies include `beatsaber-hook`, `bsml`, `custom-types`, `config-utils`, `paper2_scotland2`, `songcore`, `song-details`, `web-utils`, and `beatsaverplusplus`.
- Preserve caret dependency ranges (`^x.y.z`) in `qpm.json` and generated QMOD dependency requirements. Do not replace them with exact pins (`=x.y.z`) just to match the local cache or a build's resolved versions.
- A concrete resolved version in `qpm.shared.json` is different from a dependency range; let QPM manage resolved metadata. Do not turn resolved versions into ranges or manually rewrite generated metadata to pin dependencies.

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
- Use Unity-aware alive checks before accessing Unity objects; `UnityW<T>` alone does not provide GC ownership (see below).

## Threading And Unity Safety

- Treat Unity and HMUI objects as main-thread-only unless nearby code proves otherwise.
- Use `BSML::MainThreadScheduler::Schedule(...)` when returning from background work to UI/game objects.
- Coroutines are commonly started with `BSML::SharedCoroutineStarter` and `custom_types::Helpers::CoroutineHelper::New(...)`.
- `DataHolder` owns a lot of search/cache behavior; inspect `include/DataHolder.hpp` and `src/DataHolder.cpp` before changing search flow or song data lifetime.

## Managed GC And Lifetime Safety

These are the basic ownership rules from the workspace `../AGENTS.md` GC investigations:

- Raw managed pointers, `UnityW`, native globals/containers, lambda captures and native coroutine frames do not root managed objects. A non-null pointer is not proof that the GC has kept its target alive.
- Keep explicit strong roots, such as `safe_ptr<T*>` from beatsaber-hook, for managed objects retained by native state. Root objects before handing them to a worker, scheduling a callback, or suspending a coroutine, and retain the root until the final use. This includes managed tasks, cancellation tokens, results and UI references.
- GC survival and Unity object lifetime are separate: a rooted managed wrapper can outlive its destroyed native Unity object. Check Unity liveness again when a scheduled callback executes; rooting does not make Unity/HMUI access safe on a worker thread.
- Give retained roots an explicit owner and teardown path. On disable/destruction as appropriate, cancel outstanding work, invalidate stale callbacks, unsubscribe events, clear roots and release native state. Do not let a queued callback dereference a destroyed controller or a captured native `this` after its owner is gone.
- Never call virtual `Finalize()` from a `DECLARE_DTOR` body: it can dispatch back into the same destructor and recurse. Avoid custom finalizers when there is no native resource to release, and do not perform Unity API work on the GC finalizer thread.
- Build success does not validate GC behavior. For lifetime changes, verify repeated view open/close, scene transitions and delayed callbacks on device; distinguish that evidence from source/build checks.

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
  - For a release QMOD: `pwsh ./createqmod.ps1 BetterSongSearch -release`

## Packaging

- `mod.template.json` is the template for mod metadata.
- QPM output is `BetterSongSearch.qmod`.
- The built mod library is `libBetterSongSearch.so`.
- The `.so` is packaged as a late mod file.
- Keep metadata valid and minimal. Do not change the established mod ID.
- Run packaging commands from this repository's root. `createqmod.ps1` builds first, runs `qpm qmod manifest`, validates the generated `mod.json` against the QMOD schema, and archives the manifest, cover and listed binaries. `qpm qmod manifest` alone does not build or package a QMOD.
- Use `mod.template.json` and QPM metadata as the sources; do not hand-edit generated `mod.json` or replace its `^` dependency requirements with `=` pins.
- After packaging, check the archive opens, contains every manifest-listed file and no obsolete payload, and that its `libBetterSongSearch.so` matches `build/libBetterSongSearch.so` byte-for-byte (or by SHA-256). Check the intended game/package version and late-mod entry. The script updates an existing archive, so stale entries must not be assumed absent.
- Report build/package validation separately from device testing. Creating a QMOD does not install it or prove its runtime behavior.

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
