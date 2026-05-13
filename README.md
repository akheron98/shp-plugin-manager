# SHP Plugin Manager

A Windows desktop application that discovers, installs, updates, and uninstalls the [SHP audio plugin suite](https://github.com/akheron98/shp-plugin-registry).

Built in C++ with [JUCE](https://juce.com/) (same stack as the plugins themselves), matching the dark industrial visual identity of the SHP rack.

## Download

Grab the latest installer from [Releases](https://github.com/akheron98/shp-plugin-manager/releases/latest):

- `SHPPluginManager-Setup-vX.Y.Z.exe` — Inno Setup installer (recommended)
- `SHPPluginManager-vX.Y.Z.exe` — standalone executable

The app installs per-user under `%LOCALAPPDATA%\Programs\SHP\Plugin Manager` (no admin required).

> **Windows SmartScreen warning** — the binaries are not code-signed, so SmartScreen will show a "Windows protected your PC" panel the first time you run them. Click *More info* → *Run anyway*. Code signing is on the roadmap.

## What it does

- Reads the public [`shp-plugin-registry`](https://github.com/akheron98/shp-plugin-registry) manifest
- Queries GitHub Releases on the public [`shp-builds`](https://github.com/akheron98/shp-builds) repo to find the latest version of each plugin
- Downloads + installs `.vst3` bundles to your VST3 directory (per-user by default, system-wide opt-in)
- Tracks installed versions and offers updates when newer releases appear
- Notifies you when a new version of the manager itself is available

## Build from source

Requires Visual Studio 2022 (C++ tools) and CMake 3.22+.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Output: `build/SHPPluginManager_artefacts/Release/SHP Plugin Manager.exe`

## License

(c) Sombre Harfang Productions
