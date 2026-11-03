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
