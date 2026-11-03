# Workflow: Building the Markdown Renderer in Qt 6 / C++ with Claude Code

The work is split into small phases. Each phase has an exact prompt, acceptance criteria, and a verification step. Claude Code cannot see a GUI, so the workflow gives it three ways to check its own work: tests, an offscreen screenshot it can read, and CI on both target platforms.

---

## 0. Stack Decisions

Lock these decisions before you start. They go into `CLAUDE.md`.

| Concern | Choice | Why |
|---|---|---|
| Toolkit | **Qt 6.8 LTS, Widgets** | Native menu bar, `QSystemTrayIcon`, and small binaries. QML adds nothing for this UI. |
| Markdown | **md4c → HTML → `QTextDocument`** (via FetchContent) | Fast, CommonMark + GFM tables. Avoids QtWebEngine, which adds about 150 MB and makes Linux deployment painful. |
| Preview | `QTextBrowser` with the default stylesheet | Native rendering and selectable text. |
| PDF | `QTextDocument::print(QPdfWriter*)` | No print dialog and no extra dependency. Written straight to `~/Downloads`. |
| Storage | One `<uuid>.md` file per buffer in `QStandardPaths::AppDataLocation/buffers`, written with `QSaveFile` | Atomic writes, and the buffers are real files the user can grep. |
| Build | CMake ≥ 3.21 with presets and Ninja | Same commands locally and in CI. |
| Tests | Qt Test with `QT_QPA_PLATFORM=offscreen` | Headless on both platforms. |
| macOS package | `.app` → `macdeployqt` → ad-hoc codesign → `.dmg` (arm64, macOS 12+) | |
| Linux package | `linuxdeploy` + Qt plugin → AppImage, built on Ubuntu 22.04 | Built against an older glibc so it runs on more distros. |
| CI | GitHub Actions: `macos-14` (arm64) and `ubuntu-22.04` (amd64) | You cannot cross-compile macOS from Linux, so CI is the source of truth for both. |

---

## 1. Repository Layout

```
md/
├── CLAUDE.md                     # project memory: rules, commands, architecture
├── docs/PLAN.md                  # phase checklist that Claude updates
├── CMakeLists.txt
├── CMakePresets.json
├── .clang-format
├── .claude/
│   ├── settings.json             # permissions + hooks
│   ├── hooks/format.sh
│   ├── commands/                 # custom slash commands
│   │   ├── phase.md  build.md  shot.md  review.md  release-check.md
│   └── agents/
│       └── qt-reviewer.md        # review subagent
├── src/
│   ├── main.cpp
│   ├── core/                     # NO QtWidgets dependency
│   │   ├── Buffer.h/.cpp  BufferStore.h/.cpp  Title.h/.cpp
│   │   ├── Markdown.h/.cpp  PdfExporter.h/.cpp  Theme.h
│   ├── ui/
│   │   ├── MainWindow.*  BufferListDelegate.*  OverlayBar.*
│   │   ├── EditorView.*  PreviewView.*
│   └── platform/
│       ├── Tray.*  AppMenu.*  SingleInstance.*
│       └── mac/Dock.mm           # Objective-C++: hide dock icon, activation
├── resources/
│   ├── resources.qrc  preview.css  print.css
│   ├── icons/tray-mac.png  tray-mac@2x.png  tray-linux.png  md.icns  md.png
├── tests/
│   ├── tst_title.cpp  tst_bufferstore.cpp  tst_markdown.cpp  tst_pdf.cpp
│   └── fixtures/
├── packaging/
│   ├── macos/Info.plist.in  package.sh
│   └── linux/md.desktop  package.sh  Dockerfile
└── .github/workflows/build.yml
```

---

## 2. Bootstrap

Create these files by hand, or paste this section into Claude Code as the first prompt. They define how Claude behaves for the rest of the project.

### Prerequisites

```bash
# macOS (Apple Silicon)
brew install qt cmake ninja clang-format jq

# Linux (Ubuntu 22.04/24.04). The distro Qt is too old, so use aqtinstall.
sudo apt install build-essential cmake ninja-build clang-format jq \
  libgl1-mesa-dev libxkbcommon-x11-0 libxcb-cursor0 libfuse2 python3-pip
pip install aqtinstall && aqt install-qt linux desktop 6.8.2 linux_gcc_64 -O ~/Qt
export CMAKE_PREFIX_PATH=~/Qt/6.8.2/gcc_64
```

### `CLAUDE.md`

````markdown
# md: ultraminimal markdown buffers (Qt 6.8 / C++17)

## Product rules (never violate)
- One pane at a time: editor OR preview. No split view.
- No save button. Every edit persists (debounced 300 ms, QSaveFile). No open/save dialogs.
- Buffer title = first non-empty line with leading `#` and inline `*_\`` stripped, else "untitled". No rename.
- An empty buffer is deleted when the user switches away from it.
  `+` while the current buffer is empty reuses it.
- Chrome (overlay "edit / pdf ↓") is 25% opacity, 100% on hover. No icons in the main window.
- PDF export writes directly to ~/Downloads/<title>.pdf (dedupe with " (2)") and
  shows "saved · reveal" in the overlay for 3 s.
- All colours, fonts, and spacing come from src/core/Theme.h. Never hardcode them elsewhere.

## Commands
- Configure+build: `cmake --preset dev && cmake --build --preset dev`
- Test:            `ctest --preset dev --output-on-failure`
- Screenshot:      `./scripts/shot.sh [edit|view] [light|dark]` → writes /tmp/md-shot.png
- Format:          automatic via hook (clang-format)

## Architecture
- src/core: model + rendering. Links Qt::Core, Qt::Gui, md4c-html. NEVER include QtWidgets here.
- src/ui: widgets only. No file IO; talk to BufferStore.
- src/platform: tray, menu bar, single instance, macOS Objective-C++ (.mm).
- BufferStore is a QAbstractListModel sorted by `updated` desc. It owns all disk IO.
- `--data-dir <path>` overrides storage. Tests and screenshots MUST use a temp dir and
  never touch the user's real buffers.

## Conventions
- C++17, Qt 6.8 API, `Ctrl+X` key sequences (Qt maps them to ⌘ on macOS).
- New core logic needs a Qt Test in tests/. Run ctest before saying a task is done.
- Platform code goes behind `#ifdef Q_OS_MACOS` / `Q_OS_LINUX` in src/platform only.
- Do not add dependencies or QtWebEngine without asking.
- Commit at the end of each phase: `phase N: <summary>`. Never push.

## Verifying UI
You cannot see the window. After any UI change, run ./scripts/shot.sh for the affected
mode and theme, then read /tmp/md-shot.png and critique it against the product rules.
````

### `.claude/settings.json`

```json
{
  "permissions": {
    "allow": [
      "Bash(cmake:*)", "Bash(ctest:*)", "Bash(ninja:*)",
      "Bash(./scripts/*)", "Bash(./build/*)", "Bash(./packaging/*)",
      "Bash(clang-format:*)", "Bash(git status)", "Bash(git diff:*)",
      "Bash(git add:*)", "Bash(git commit:*)", "Bash(git log:*)",
      "Bash(docker build:*)", "Bash(docker run:*)"
    ],
    "deny": ["Bash(git push:*)", "Bash(rm -rf ~*)", "Bash(sudo:*)"]
  },
  "hooks": {
    "PostToolUse": [
      {
        "matcher": "Edit|Write|MultiEdit",
        "hooks": [{ "type": "command", "command": ".claude/hooks/format.sh" }]
      }
    ]
  }
}
```

### `.claude/hooks/format.sh`

```bash
#!/usr/bin/env bash
# Formats C++/ObjC++ files after Claude edits them.
f=$(jq -r '.tool_input.file_path // empty')
case "$f" in *.cpp|*.h|*.mm) clang-format -i "$f" ;; esac
exit 0
```

### Custom Slash Commands

**`.claude/commands/phase.md`**

```markdown
Read CLAUDE.md and docs/PLAN.md. Implement **Phase $ARGUMENTS** only.
1. Present a short plan (files to add or change, tests to write) and wait for my approval.
2. Write the tests first where the phase has core logic.
3. Implement, build, and run ctest until green. For UI work, run ./scripts/shot.sh and inspect the image.
4. Tick the phase's checkboxes in docs/PLAN.md, then commit `phase $ARGUMENTS: ...`.
5. Report: what was done, what you verified, and anything I must check by hand on real hardware.
```

**`.claude/commands/build.md`**

```markdown
Run `cmake --preset dev && cmake --build --preset dev && ctest --preset dev --output-on-failure`.
Fix any failure and repeat until green. Summarise in 3 lines.
```

**`.claude/commands/shot.md`**

```markdown
Run ./scripts/shot.sh $ARGUMENTS, read /tmp/md-shot.png, and critique it against the
product rules in CLAUDE.md: spacing, faint chrome, typography, sidebar, dark mode.
List concrete fixes, apply them, then re-shoot once.
```

**`.claude/commands/review.md`**

```markdown
Use the qt-reviewer subagent to review `git diff main...HEAD`. Apply the fixes it marks as must-fix.
```

**`.claude/commands/release-check.md`**

```markdown
Go through the "Manual QA" list in docs/PLAN.md. For each item, state whether it is
covered by an automated test, a screenshot, or needs a human. Produce a checklist for me.
```

### `.claude/agents/qt-reviewer.md`

```markdown
---
name: qt-reviewer
description: Reviews Qt/C++ diffs for ownership, threading, cross-platform, and product-rule violations.
tools: Read, Grep, Glob, Bash
---
You are a senior Qt engineer. Review the diff for:
- QObject parent/ownership leaks, dangling `this` captures in lambdas (connect with a context object).
- UI-thread blocking IO, and missing QSaveFile commit() error handling.
- QtWidgets included in src/core.
- Platform code outside src/platform, and missing #ifdef guards.
- Shortcut conflicts (e.g. Ctrl+Up inside the editor), missing QAction::MenuRole on macOS.
- Violations of the product rules in CLAUDE.md.
Output two lists: MUST-FIX and NICE-TO-HAVE, each with file:line references.
```

### `scripts/shot.sh`

This script is how Claude "sees" the UI.

```bash
#!/usr/bin/env bash
set -euo pipefail
MODE=${1:-view}; THEME=${2:-light}
DIR=$(mktemp -d); cp tests/fixtures/*.md "$DIR"/
./build/dev/md${APP_SUFFIX:-} --data-dir "$DIR" --screenshot /tmp/md-shot.png \
  --mode "$MODE" --theme "$THEME" --size 1100x700
echo "/tmp/md-shot.png"
```

On macOS the binary is at `build/dev/md.app/Contents/MacOS/md`. Have Claude make the script detect this in Phase 3.

---

## 3. Build Scaffolding

### `CMakePresets.json`

```json
{
  "version": 6,
  "configurePresets": [
    { "name": "base", "hidden": true, "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": { "CMAKE_EXPORT_COMPILE_COMMANDS": "ON" } },
    { "name": "dev", "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "release-mac", "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_OSX_ARCHITECTURES": "arm64",
        "CMAKE_OSX_DEPLOYMENT_TARGET": "12.0" } },
    { "name": "release-linux", "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_INSTALL_PREFIX": "/usr" } }
  ],
  "buildPresets": [
    { "name": "dev", "configurePreset": "dev" },
    { "name": "release-mac", "configurePreset": "release-mac" },
    { "name": "release-linux", "configurePreset": "release-linux" }
  ],
  "testPresets": [
    { "name": "dev", "configurePreset": "dev", "output": { "outputOnFailure": true } },
    { "name": "release-mac", "configurePreset": "release-mac" },
    { "name": "release-linux", "configurePreset": "release-linux" }
  ]
}
```

### `CMakeLists.txt` (starting point)

```cmake
cmake_minimum_required(VERSION 3.21)
project(md VERSION 0.1.0 LANGUAGES CXX)
if(APPLE)
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Test)
qt_standard_project_setup()

# md4c (CommonMark + GFM)
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_MD2HTML_EXECUTABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(md4c GIT_REPOSITORY https://github.com/mity/md4c.git GIT_TAG release-0.5.2)
FetchContent_MakeAvailable(md4c)

# Core library (no widgets)
add_library(md_core STATIC
  src/core/Buffer.cpp src/core/BufferStore.cpp src/core/Title.cpp
  src/core/Markdown.cpp src/core/PdfExporter.cpp)
target_include_directories(md_core PUBLIC src)
target_link_libraries(md_core PUBLIC Qt6::Core Qt6::Gui md4c-html)

# App
qt_add_executable(md MACOSX_BUNDLE
  src/main.cpp
  src/ui/MainWindow.cpp src/ui/BufferListDelegate.cpp src/ui/OverlayBar.cpp
  src/ui/EditorView.cpp src/ui/PreviewView.cpp
  src/platform/Tray.cpp src/platform/AppMenu.cpp src/platform/SingleInstance.cpp)
qt_add_resources(md "res" PREFIX "/" BASE resources FILES
  resources/preview.css resources/print.css
  resources/icons/tray-mac.png resources/icons/tray-mac@2x.png
  resources/icons/tray-linux.png resources/icons/md.png)
target_link_libraries(md PRIVATE md_core Qt6::Widgets Qt6::Network)

if(APPLE)
  target_sources(md PRIVATE src/platform/mac/Dock.mm resources/icons/md.icns)
  set_source_files_properties(resources/icons/md.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_link_libraries(md PRIVATE "-framework AppKit")
  set_target_properties(md PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER dev.yourname.md
    MACOSX_BUNDLE_ICON_FILE md.icns
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
endif()

install(TARGETS md BUNDLE DESTINATION . RUNTIME DESTINATION bin)
if(UNIX AND NOT APPLE)
  install(FILES packaging/linux/md.desktop DESTINATION share/applications)
  install(FILES resources/icons/md.png DESTINATION share/icons/hicolor/256x256/apps)
endif()

# Tests (headless)
enable_testing()
foreach(t title bufferstore markdown pdf)
  qt_add_executable(tst_${t} tests/tst_${t}.cpp)
  target_link_libraries(tst_${t} PRIVATE md_core Qt6::Test)
  add_test(NAME ${t} COMMAND tst_${t})
  set_tests_properties(${t} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()
```

---

## 4. `docs/PLAN.md`: The Phases

Each phase is one Claude Code session. Run `/phase N`, approve the plan, let it run, review the result, then `/clear` before the next phase. The PLAN file and `CLAUDE.md` carry the context forward, so the conversation doesn't need to.

### Phase 0: Skeleton + CI Green on Both Platforms

Set up CI first. Platform breakage is cheap to fix on day 1 and expensive on day 10.

> **Prompt:** `/phase 0`. Create the CMake project, an empty `QMainWindow` titled "md", one trivial test, `scripts/shot.sh`, and `.github/workflows/build.yml` (from section 6). Add `--data-dir`, `--screenshot`, `--mode`, `--theme`, and `--size` CLI flags in `main.cpp`. If `--screenshot` is present, call `qputenv("QT_QPA_PLATFORM","offscreen")` **before** constructing `QApplication`. Then show the window, process events, `grab().save(path)`, and exit 0.

- [ ] `cmake --preset dev` builds locally
- [ ] `ctest` passes offscreen
- [ ] `shot.sh` produces a PNG that Claude reads
- [ ] CI is green on `macos-14` and `ubuntu-22.04` (you push; Claude never pushes)

### Phase 1: Core Model (Test-First)

> **Prompt:** `/phase 1`. Implement `Title::fromText`, `Buffer`, and `BufferStore` (a `QAbstractListModel`).
> - Storage is `<dataDir>/buffers/<uuid>.md`, and `updated` is the file mtime.
> - Write with `QSaveFile`, debounced 300 ms per buffer.
> - Implement `flush()` on quit.
> - Implement `create()`, `setText()`, `remove()`, and `pruneIfEmpty(id)`.
> - Sorting is by `updated` descending; emit `rowsMoved` rather than resetting the model.
> - Use `QFileSystemWatcher` to reload buffers edited externally, unless there are local unsaved changes.
>
> Write the tests first.

- [ ] Title edge cases: blank lines, `###`, emphasis markers, truncation at 60 characters, "untitled"
- [ ] Round-trip: create → setText → new store instance reads the same text
- [ ] Debounce: 10 rapid `setText` calls produce 1 write. Use `QTest::qWait` and count via mtime or a write hook.
- [ ] `pruneIfEmpty` deletes the file; `+` on an empty current buffer reuses it
- [ ] An external edit is picked up within 1 s

### Phase 2: Markdown Rendering + PDF

> **Prompt:** `/phase 2`. Implement `Markdown::toHtml(QString)` using md4c-html with `MD_DIALECT_GITHUB` flags. Implement `Theme.h`: light and dark palettes, plus the serif, mono, and UI font stacks. Fallbacks: Charter → Iowan → Georgia → serif; SF Mono → JetBrains Mono → monospace. Write `preview.css` and `print.css` using **only the CSS subset `QTextDocument` supports**. Implement `PdfExporter::exportTo(text, title, path)`:
> - Use `QPdfWriter` with A4, 20 mm margins, and 300 dpi.
> - Use a separate `QTextDocument` with `print.css`.
> - Set the title metadata.
>
> Add `PdfExporter::downloadsPath(title)`, which resolves `QStandardPaths::DownloadLocation` and dedupes the filename with " (2)", " (3)", and so on.

- [ ] HTML tests: headings, GFM table, fenced code, and an escaped `<script>` that must not pass through raw
- [ ] The PDF test writes a file that starts with `%PDF` and is larger than 1 KB, and a 200-line document produces more than 1 page
- [ ] Dedupe test

### Phase 3: Main Window (Visual Loop)

> **Prompt:** `/phase 3`. Build the main window:
> - **Sidebar:** 220 px. `QListView` plus `BufferListDelegate`, which paints the title, the relative time (just now / 5m / 3h / 2d / "Mar 4"), and a hover-only `×`.
> - **Header:** a `+` label at the top of the sidebar.
> - **Main area:** a `QStackedWidget` holding `EditorView` (a `QPlainTextEdit` with mono 15 px, 1.75 line height, a 72-character centred column with large padding, and no frame) and `PreviewView` (a `QTextBrowser` with a serif 68-character column and external links opening in the browser).
> - **OverlayBar:** a child of the central widget, anchored top-right. Its opacity animates between 0.25 and 1 on hover using `QGraphicsOpacityEffect` and `QPropertyAnimation`.
> - **Dark mode:** follow `QStyleHints::colorScheme()` and react to `colorSchemeChanged`.
>
> After each visual change, run `/shot view light`, `/shot edit light`, and `/shot view dark`, then iterate.

- [ ] All three screenshots match the HTML prototype's feel. You also eyeball them yourself.
- [ ] No frames, borders, or focus rings visible in the main pane
- [ ] Relative time refreshes every 60 s

### Phase 4: Interactions + Native Menu Bar

> **Prompt:** `/phase 4`. Wire the behaviour.
>
> In `AppMenu`, build a `QMenuBar` from shared `QAction`s:
>
> | Menu | Actions |
> |---|---|
> | **File** | New Buffer `Ctrl+N` · Export PDF `Ctrl+P` · Delete Buffer `Ctrl+Backspace` · Reveal Buffers Folder · Quit (`QuitRole`) |
> | **Edit** | Standard Undo, Redo, Cut, Copy, Paste, and Select All, forwarded to the editor |
> | **View** | Toggle Edit/Preview `Ctrl+E` · Toggle Sidebar `Ctrl+\` (animate `maximumWidth`) |
> | **Go** | Previous `Ctrl+Up` · Next `Ctrl+Down`. Enabled only in preview mode, so they don't clash with editor navigation. |
> | **Window** | Minimise, Zoom (macOS only) |
> | **Help** | About md (`AboutRole`) |
>
> The overlay buttons trigger the same actions. Also implement:
> - Double-clicking the preview switches to edit mode.
> - Tab in the editor inserts 2 spaces.
> - Opening a buffer shows the preview; a new buffer opens in edit mode.
> - Switching buffers calls `pruneIfEmpty` on the buffer being left.
> - Delete asks for confirmation only when the buffer is non-empty, using a minimal `QMessageBox`.
> - PDF export writes to Downloads, then the overlay shows "saved · reveal" for 3 s. Reveal runs `open -R` on macOS and `QDesktopServices::openUrl(folder)` on Linux.

- [ ] On macOS, Quit and About land in the application menu (check the roles)
- [ ] Shortcuts show ⌘ glyphs on macOS and Ctrl on Linux
- [ ] Widget tests cover the toggle, prune-on-switch, and the export action creating a file in a temp Downloads override

### Phase 5: Tray Icon / Menu Bar Extra + Lifecycle

> **Prompt:** `/phase 5`. Implement `Tray` using `QSystemTrayIcon`.
> - **Icon:** on macOS use `tray-mac.png` (black with alpha, 18 pt with an @2x version) and call `icon.setIsMask(true)` so it becomes a template image that adapts to the menu bar. On Linux use `tray-linux.png` at 22 px.
> - **Tray menu:** New Buffer · the 5 most recent buffers by title (click to open) · separator · Show/Hide md · Quit.
> - **Activation:** on Linux, a `Trigger` activation toggles the window. On macOS, a click shows the menu (the native behaviour).
>
> Lifecycle:
> - Call `setQuitOnLastWindowClosed(false)` only if `QSystemTrayIcon::isSystemTrayAvailable()`.
> - Closing the window then hides it. If no tray is available (for example, GNOME without the AppIndicator extension), closing quits normally.
> - macOS dock: when the app becomes active (`applicationStateChanged` → `Active`) with no visible window, show it.
> - `mac/Dock.mm` adds an optional "Hide Dock Icon" setting using `[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory]` / `Regular`.
> - `SingleInstance` uses `QLocalServer` named `md-<username>`. A second launch sends "show" and exits.
> - Persist window geometry, sidebar state, and the last buffer in `QSettings`.
> - Call `BufferStore::flush()` on `aboutToQuit`.

- [ ] Quitting from the tray flushes pending edits (test: type, then quit within 100 ms, then reopen)
- [ ] Launching the app twice focuses the existing window
- [ ] **Manual:** the macOS menu bar icon is legible in light and dark menu bars; the tray shows on KDE and on GNOME with the AppIndicator extension

### Phase 6: Platform Polish

> **Prompt:** `/phase 6`.
> - **macOS:** add `Info.plist.in` with `LSMinimumSystemVersion` 12.0, `NSHighResolutionCapable`, and `LSApplicationCategoryType` = productivity. Optionally make the title bar transparent with full-size content view in `Dock.mm` so the sidebar runs to the top, keeping the traffic lights inset.
> - **Linux:** add the `.desktop` file (`StartupWMClass=md`, so the window groups with its launcher) and `QGuiApplication::setDesktopFileName("md")`. Test on both Wayland and X11 using `QT_QPA_PLATFORM=xcb`.
> - Add `packaging/linux/Dockerfile` (Ubuntu 22.04 + aqt Qt 6.8.2) so that when you develop on a Mac, Claude can run the Linux build and tests: `docker build -f packaging/linux/Dockerfile . && docker run --rm ... ctest`.

- [ ] Linux build and tests pass in Docker, run from a Mac host
- [ ] **Manual:** test on a Retina and a non-Retina display, and under fractional scaling on Linux

### Phase 7: Packaging + Release

> **Prompt:** `/phase 7`. Write `packaging/macos/package.sh` and `packaging/linux/package.sh` (section 7). Extend CI to package and upload artifacts. On `v*` tags, create a GitHub Release with the `.dmg` and the `.AppImage`. Add optional notarization steps gated on the `APPLE_*` secrets being present.

- [ ] The `.dmg` opens, and the app launches on a clean Mac. Without notarization, the user must right-click → Open, or run `xattr -cr md.app`.
- [ ] The AppImage runs on Ubuntu 22.04 and 24.04, and on Fedora; tray and PDF export both work

### Phase 8: Hardening

> **Prompt:** `/phase 8`.
> - Run large-file tests: a 5 MB buffer must type without lag. Render the preview lazily, only when switching to view mode.
> - Recover from a corrupted or unreadable file by skipping it and logging.
> - Handle a read-only data dir with a visible, non-modal error.
> - Run with the address sanitizer in a `dev-asan` preset.
> - Then run `/review` and fix everything marked MUST-FIX.

**Manual QA (final):**
- [ ] Edit, force-quit (`kill -9`), relaunch: at most 300 ms of edits are lost
- [ ] Edit the same buffer from an external editor while the app is open
- [ ] Switch to dark mode live
- [ ] Hold down the `+` key: no buffers pile up
- [ ] Export a PDF of a 30-page document: page breaks are sensible, code blocks are readable
- [ ] Quit and relaunch: you return to the same buffer and mode

---

## 5. Implementation Notes for Tricky Parts

Give these to Claude when the matching phase starts. They are short snippets for the non-obvious parts.

**Screenshot mode (main.cpp).** `QT_QPA_PLATFORM` must be set before the app object exists:

```cpp
int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (qstrcmp(argv[i], "--screenshot") == 0) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QApplication::setApplicationName("md");
    QApplication::setOrganizationName("md");
    QGuiApplication::setDesktopFileName("md");
    // ... parse with QCommandLineParser; if screenshot:
    //   w.resize(size); w.show(); QCoreApplication::processEvents();
    //   w.grab().save(path); return 0;
}
```

**Template tray icon on macOS:**

```cpp
QIcon icon;
#ifdef Q_OS_MACOS
icon.addFile(":/icons/tray-mac.png");
icon.addFile(":/icons/tray-mac@2x.png");
icon.setIsMask(true);              // macOS tints it for light/dark menu bars
#else
icon = QIcon(":/icons/tray-linux.png");
#endif
tray->setIcon(icon);
```

**Close to tray, without trapping Linux users who have no tray:**

```cpp
void MainWindow::closeEvent(QCloseEvent* e) {
    if (m_tray && m_tray->isVisible()) { hide(); e->ignore(); }
    else { m_store->flush(); e->accept(); }
}
```

**PDF without a print dialog:**

```cpp
bool PdfExporter::exportTo(const QString& md, const QString& title, const QString& path) {
    QPdfWriter pdf(path);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(20, 22, 20, 22), QPageLayout::Millimeter);
    pdf.setResolution(300);
    pdf.setTitle(title);
    QTextDocument doc;
    doc.setDefaultStyleSheet(loadResource(":/print.css"));
    doc.setDefaultFont(Theme::serifFont(11.5));
    doc.setHtml(Markdown::toHtml(md));
    doc.setPageSize(pdf.pageLayout().paintRectPixels(pdf.resolution()).size());
    doc.print(&pdf);
    return QFileInfo(path).size() > 0;
}
```

**Faint overlay:**

```cpp
auto* fx = new QGraphicsOpacityEffect(this); fx->setOpacity(0.25); setGraphicsEffect(fx);
m_anim = new QPropertyAnimation(fx, "opacity", this); m_anim->setDuration(180);
// enterEvent → animate to 1.0, leaveEvent → animate to 0.25
```

**Known limits of the Qt rich-text CSS subset.** Tell Claude these up front so it doesn't fight them:
- There is no `border-radius` and no `max-width`. Centre the text column with `QTextDocument::setDocumentMargin` plus viewport margins computed from the widget width in `resizeEvent`.
- Use `<pre>` background colour, `margin`, and `line-height` (Qt 6 supports `line-height` as a percentage).

---

## 6. CI: `.github/workflows/build.yml`

```yaml
name: build
on:
  push: { branches: [main], tags: ['v*'] }
  pull_request:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - { os: macos-14,     preset: release-mac,   pkg: macos }   # arm64 runner
          - { os: ubuntu-22.04, preset: release-linux, pkg: linux }   # amd64, old glibc
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - uses: jurplel/install-qt-action@v4
        with: { version: '6.8.2', cache: true }

      - name: Deps (Linux)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y ninja-build libgl1-mesa-dev libxkbcommon-x11-0 \
            libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 libxcb-shape0 libfuse2

      - name: Deps (macOS)
        if: runner.os == 'macOS'
        run: brew install ninja

      - run: cmake --preset ${{ matrix.preset }}
      - run: cmake --build --preset ${{ matrix.preset }}
      - run: ctest --preset ${{ matrix.preset }} --output-on-failure

      - name: Package
        run: ./packaging/${{ matrix.pkg }}/package.sh

      - uses: actions/upload-artifact@v4
        with: { name: md-${{ matrix.pkg }}, path: dist/* }

  release:
    if: startsWith(github.ref, 'refs/tags/v')
    needs: build
    runs-on: ubuntu-latest
    permissions: { contents: write }
    steps:
      - uses: actions/download-artifact@v4
        with: { path: dist, merge-multiple: true }
      - uses: softprops/action-gh-release@v2
        with: { files: dist/* }
```

---

## 7. Packaging Scripts

**`packaging/macos/package.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
APP=build/release-mac/md.app
macdeployqt "$APP" -always-overwrite
# arm64 binaries must be signed. Ad-hoc signing works locally; use a Developer ID for distribution.
codesign --force --deep --sign "${MAC_SIGN_ID:--}" --options runtime "$APP"
mkdir -p dist
hdiutil create -volname md -srcfolder "$APP" -ov -format UDZO dist/md-macos-arm64.dmg
# Optional: xcrun notarytool submit dist/*.dmg --apple-id ... --wait && xcrun stapler staple dist/*.dmg
```

**`packaging/linux/package.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
rm -rf AppDir && DESTDIR=AppDir cmake --install build/release-linux
base=https://github.com/linuxdeploy
wget -qN $base/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -qN $base/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage
export APPIMAGE_EXTRACT_AND_RUN=1                 # no FUSE needed in CI
export QMAKE=$(command -v qmake6 || command -v qmake)
export EXTRA_PLATFORM_PLUGINS="libqwayland-generic.so;libqwayland-egl.so"
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage \
  -d packaging/linux/md.desktop -i resources/icons/md.png
mkdir -p dist && mv md*.AppImage dist/md-linux-x86_64.AppImage
```

**`packaging/linux/md.desktop`**

```ini
[Desktop Entry]
Type=Application
Name=md
Comment=Ultraminimal markdown buffers
Exec=md
Icon=md
Categories=Office;TextEditor;
StartupWMClass=md
```

---

## 8. Working Rhythm with Claude Code

```
┌─ per phase ─────────────────────────────────────────────────────────┐
│ /clear → /phase N → review plan (Shift+Tab plan mode if needed)    │
│   → approve → Claude: tests → code → /build → /shot (UI phases)    │
│   → PLAN.md ticked → commit                                         │
│ you: run app locally 2 min → /review → git push → CI green on both │
└─────────────────────────────────────────────────────────────────────┘
```

**Tips**

- **Keep context small.** Use `/clear` between phases. `CLAUDE.md` and `PLAN.md` are the memory, so Claude rereads them each time instead of relying on a long conversation.
- **Make Claude look at the UI.** The screenshot loop is what makes UI work feasible. Insist on it for every visual change: "read the PNG, critique it, fix it, re-shoot."
- **Parallelise when it's safe.** After Phase 4, packaging (Phase 7) is independent of the tray work (Phase 5). Run them in separate git worktrees with two Claude sessions: `git worktree add ../md-pkg phase-7`.
- **CI is the gate.** A phase is not done until it is green on both runners. Paste failing CI logs straight back to Claude.
- **Keep humans in the loop for anything sensory.** Check tray legibility, font rendering, Gatekeeper behaviour, and the GNOME tray yourself. `/release-check` generates that checklist for you.
- **Leave room for a fidelity upgrade.** If the Qt CSS subset ever becomes too limiting for the PDF, add `-DMD_WEBENGINE=ON` as a separate, optional phase using `QWebEnginePage::printToPdf`. Don't start with it.
