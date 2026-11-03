# md: Phase Checklist

Phases are defined in WORKFLOW.md. Tick items as they are verified.

## Phase 0: Skeleton + CI Green
- [x] `cmake --preset dev` builds locally (macOS arm64, Debug)
- [x] `ctest --preset dev` passes offscreen
- [ ] `scripts/shot.sh` produces a PNG that Claude reads (binary screenshot at 1100x700 reviewed: blank grey window, as expected for an empty QMainWindow; `shot.sh` itself still writes to /tmp, so it has not been run end-to-end inside the project)
- [ ] CI is green on `macos-14` (requires push; not done)

## Phase 1: Core Model (Test-First)
- [x] Title edge cases: blank lines, `###`, emphasis markers, truncation at 60 characters, "untitled"
- [x] Round-trip: create → setText → new store instance reads the same text
- [x] Debounce: 10 rapid `setText` calls produce 1 write
- [x] `pruneIfEmpty` deletes the file (`+` reuse is UI behaviour, deferred to Phase 4)
- [x] An external edit is picked up within 1 s

## Phase 2: Markdown Rendering + PDF
- [x] HTML tests: headings, GFM table, fenced code, escaped `<script>`
- [x] PDF test writes a file starting with `%PDF`, larger than 1 KB; 200-line document produces more than 1 page
- [x] Dedupe test

## Phase 3: Main Window (Visual Loop)
- [x] Screenshots (view/edit, light/dark) reviewed by eye in `build/` (rendered by the binary, not `shot.sh`; no prototype was provided, so "feel" is judged against CLAUDE.md)
- [x] No frames visible in the main pane (`window` test checks `NoFrame`)
- [ ] Relative time refreshes every 60 s (60 s timer implemented; not covered by a test)

## Phase 4: Interactions + Native Menu Bar
- [ ] On macOS, Quit and About land in the application menu (roles set; needs a run on macOS to confirm placement)
- [ ] Shortcuts show ⌘ glyphs on macOS (Qt maps Ctrl to ⌘; needs a manual look)
- [x] Widget tests cover toggle, prune-on-switch, and export action (plus Tab, new/reuse, delete, Go enablement, sidebar collapse)

## Phase 5: Tray Icon / Lifecycle
- [x] Quitting flushes pending edits (`tst_lifecycle` covers the flush; the tray Quit item is wired to `QCoreApplication::quit` but not exercised)
- [x] Launching the app twice focuses the existing window (`tst_singleinstance` covers the second-launch signal; `showWindow` raises the window)
- [ ] **Manual:** macOS menu bar icon legible in light and dark menu bars

## Phase 6: Platform Polish
- [x] Packaging files checked by `tst_packaging` (macOS `Info.plist.in` keys and bundle values; `md.desktop` Exec/Icon/StartupWMClass)
- [x] macOS bundle carries identifier, version and `LSMinimumSystemVersion` 12.0 (`build/dev/md.app/Contents/Info.plist`)
- [ ] Linux build and ctest pass in Docker (`packaging/linux/Dockerfile`; skipped for now: aqt could not download Qt 6.8.2 from inside the Docker build after repeated attempts)
- [ ] **Manual:** test on Retina and non-Retina displays
- [ ] **Manual:** Wayland and X11 sessions (tray and window behaviour)
- Deferred: title-bar transparency (skipped per approval)

## Phase 7: Packaging + Release
- [x] `packaging/macos/package.sh` builds `dist/md-macos-arm64.dmg` (created locally; unsigned, not notarised)
- [ ] `.dmg` opens and app launches on a clean Mac (manual)
- [x] Placeholder app icon `resources/icons/md.icns` and `md.png` wired into the bundle
- [x] README and docs (`docs/usage.md`, `docs/building.md`, `docs/architecture.md`) with screenshots in `docs/images/`
- Skipped for now by request: Linux AppImage, signing and notarisation, CI packaging and release jobs, and tests for these steps

## Phase 8: Hardening
- [ ] 5 MB buffer types without lag
- [ ] Corrupted file is skipped and logged
- [ ] Read-only data dir shows a non-modal error

**Manual QA (final):**
- [ ] Edit, force-quit (`kill -9`), relaunch: at most 300 ms of edits lost
- [ ] Edit the same buffer from an external editor while the app is open
- [ ] Switch to dark mode live
- [ ] Hold `+`: no buffers pile up
- [ ] Export a 30-page PDF: sensible page breaks, readable code blocks
- [ ] Quit and relaunch: same buffer and mode
