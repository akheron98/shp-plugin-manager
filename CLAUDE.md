# CLAUDE.md — SHP Plugin Manager

This is the **manager** sub-project. For the full operational reference covering all repos, releases, the auto-update flow, and how to add a new plugin to the suite, read [`../CLAUDE.md`](../CLAUDE.md) — that is the canonical doc.

## Local quick reference

**Build** :
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
Output: `build/SHPPluginManager_artefacts/Release/SHP Plugin Manager.exe`

**Release a new version** (see `../CLAUDE.md` §3.2 for the canonical recipe):
1. Edit `Source/Main.cpp` → `getApplicationVersion()` must return the new version
2. Commit, tag `vX.Y.Z`, push the tag
3. CI builds the .exe + Inno Setup installer, attaches both to a Release on `akheron98/shp-plugin-manager`

**Key classes** (this folder):
- `MainComponent` — UI shell, hosts cards + Settings page + update banner
- `RegistryClient` — fetches manifest from `shp-plugin-registry`, fetches Releases from `shp-builds`
- `InstallManager` — async download/extract/copy/record state machine
- `UpdateChecker` — self-update check against `shp-plugin-manager` Releases
- `Settings` — install scope (user/system/custom), GitHub PAT, registry URL
- `ElevationHelper` — UAC `runas` helper for Program Files installs
- `ShpLookAndFeel` + `Theme.h` — visual identity (palette + fonts)

**Inno Setup script**: `packaging/installer.iss`. Version is injected by CI via `/DAppVersion=`. Inno Setup 6.7.1 is preinstalled on `windows-2022`; do not `choco install` it.

**Runtime state on user machine**:
- `%APPDATA%\SHP\manager\state.json` — installed plugins record
- `%APPDATA%\SHP\manager\SHPPluginManager.settings` — user prefs
- Default VST3 install location: `%LOCALAPPDATA%\Programs\Common\VST3\<Plugin>.vst3\`
