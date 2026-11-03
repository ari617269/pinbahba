# Architecture

md is a Qt 6.8 Widgets app written in C++17. It keeps one rule above the others: the window shows one pane at a time, with no chrome beyond the overlay.

## Layout

```
src/core/       model, title, rendering, PDF export. Qt Core and Gui only (no QtWidgets).
src/ui/         widgets: MainWindow, editor, preview, sidebar delegate, overlay, menu bar.
src/platform/   tray, single instance, reveal in Finder, Dock visibility (Objective-C++ on macOS).
resources/      preview.css, print.css (Qt resources) and icons.
tests/          Qt Test suites, one executable per area.
packaging/      macOS Info.plist template and package.sh; Linux desktop entry.
```

## Core

- **`BufferStore`** is a `QAbstractListModel` sorted by last-updated time, newest first. It owns all disk I/O. Each buffer is `<data-dir>/buffers/<id>.md`.
- Writes are debounced for 300 ms and go through `QSaveFile`, so a crash mid-write does not corrupt the file.
- A `QFileSystemWatcher` reloads a buffer when it changes on disk, unless md has its own unsaved edits pending.
- **`Title`** derives the title from the text. It is the single place where title rules live.
- **`Markdown`** renders with md4c (GitHub dialect, raw HTML disabled).
- **`PdfExporter`** renders the same HTML with `print.css` to an A4 `QPdfWriter`.
- **`Theme.h`** is the only place colours, fonts and spacing are defined.

## UI

- **`MainWindow`** holds the sidebar, a `QStackedWidget` with the editor and preview, and the overlay. It restores geometry, the last buffer and the mode from `QSettings`.
- **`EditorView`** is a `QTextEdit` (plain text). A `QPlainTextEdit` ignored the line-height setting.
- **`PreviewView`** is a `QTextBrowser`. Code blocks and tables get inline styles, because Qt's stylesheet support does not reach them.
- **`OverlayBar`** fades between 25% and 100% opacity with a `QPropertyAnimation`.
- **`AppMenu`** builds the menu bar. Edit actions are forwarded to whichever editor has focus.

## Platform

- **Single instance:** a second launch connects to a `QLocalServer`, sends `show`, and exits.
- **Tray:** `QSystemTrayIcon`, shown when the platform supports it. Closing the window hides it instead of quitting.
- **Dock visibility (macOS):** `NSApp.setActivationPolicy` in `src/platform/mac/Dock.mm`.

## Design rules

These are enforced by the tests or by review. Full product rules are in `CLAUDE.md`.

- One pane at a time. No split view.
- No save button, no open or save dialogs. Every edit persists.
- Chrome is 25% opacity, 100% on hover.
- An empty buffer is deleted when you switch away from it.
- Colours, fonts and spacing come from `Theme.h`.

## Tests

| Suite | Covers |
| --- | --- |
| `title` | Title rules, truncation, `untitled` |
| `bufferstore` | Round trip, debounce, prune, external edits |
| `markdown` | HTML output, escaping |
| `pdf` | Header, page count, dedupe |
| `relativetime` | Time labels |
| `lifecycle` | Quit flushes pending edits |
| `window` | Modes, shortcuts, sidebar, export, tray hooks |
| `singleinstance` | Second launch notifies the first |
| `packaging` | Packaging files have the keys and entries they need |

Run them with `ctest --preset dev`. See [building.md](building.md).
