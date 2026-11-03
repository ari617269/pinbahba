# Building, testing and packaging

## Requirements

- macOS 12 or later on Apple silicon (arm64), Xcode command line tools
- Qt 6.8 (Homebrew: `brew install qt ninja`)
- CMake 3.25 or later, Ninja
- Internet access on the first configure: md4c 0.5.2 is fetched with CMake `FetchContent`

On Apple silicon, if your shell runs under Rosetta, invoke the arm64 tools explicitly:

```sh
arch -arm64 /opt/homebrew/bin/cmake --preset dev
```

## Build

```sh
cmake --preset dev
cmake --build --preset dev
```

The app bundle is written to `build/dev/md.app`.

## Test

```sh
ctest --preset dev --output-on-failure
```

Tests run offscreen (`QT_QPA_PLATFORM=offscreen`) and use temporary data directories, so they never touch your real buffers.

## Run with a separate data directory

```sh
build/dev/md.app/Contents/MacOS/md \
  --data-dir /tmp/md-data \
  --downloads-dir /tmp/md-downloads \
  --settings-file /tmp/md.ini
```

| Option | Purpose |
| --- | --- |
| `--data-dir <path>` | Where buffers are stored |
| `--downloads-dir <path>` | Where PDFs are written |
| `--settings-file <path>` | INI file for window state and preferences |
| `--mode edit\|view` | Starting mode |
| `--theme system\|light\|dark` | Colour theme (default `system`) |
| `--size WxH` | Window size |
| `--screenshot <path>` | Save a PNG of the window and exit |

## Screenshots

The images in `docs/images/` are rendered by the binary itself:

```sh
B=build/dev/md.app/Contents/MacOS/md
$B --data-dir build/shot-data --settings-file build/shot.ini \
   --mode view --theme light --size 1100x700 --screenshot docs/images/view-light.png
```

## Package a macOS `.dmg`

```sh
packaging/macos/package.sh
```

The script configures a Release build in `build/release-mac`, runs `macdeployqt` to bundle Qt, and writes `dist/md-macos-arm64.dmg`. The build is unsigned and not notarised; see [usage.md](usage.md#gatekeeper-unsigned-builds) for how to open it.

## Icons

The app icon lives in `resources/icons/md.icns` (macOS) and `resources/icons/md.png`. Both are placeholders generated from a script; replace them with real artwork before a public release. The tray icons (`tray-mac*.png`, `tray-linux.png`) are also placeholders.

## Linux

A Linux build is planned but not yet verified. The Qt and CMake setup is portable, but the Docker check has not been completed.
