#pragma once

#include <optional>

#include <QMainWindow>

#include "core/Theme.h"

class BufferListDelegate;
class BufferStore;
class EditorView;
class OverlayBar;
class PreviewView;
class QHideEvent;
class QListView;
class QModelIndex;
class QParallelAnimationGroup;
class QSettings;
class QShowEvent;
class QStackedWidget;
class QWidget;
class Tray;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class Mode { Edit, View };
    enum class ThemeChoice { System, Light, Dark };

    MainWindow(BufferStore* store, std::optional<Mode> mode, ThemeChoice theme,
               const QString& downloadsDir, QSettings* settings, QWidget* parent = nullptr);

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);
    void toggleMode();
    void toggleSidebar();
    void newBuffer();
    void deleteBuffer();
    void exportPdf();
    void revealLastExport();
    void revealBuffersFolder();
    void selectRelative(int delta);
    void openBuffer(const QString& id);
    void showWindow();
    void toggleWindow();
    bool enableTray();
    void saveState();
    bool dockIconHidden() const;
    void setDockIconHidden(bool hidden);

signals:
    void modeChanged(MainWindow::Mode mode);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    Theme::Palette resolvedPalette() const;
    void applyTheme();
    void showBuffer(const QModelIndex& index);
    void onEditorTextChanged();
    void positionOverlay();
    void refreshTrayRecents();

    BufferStore* m_store;
    ThemeChoice m_themeChoice;
    QString m_downloadsDir;
    QSettings* m_settings;
    QString m_lastExport;
    Mode m_mode = Mode::View;
    QString m_currentId;
    bool m_sidebarVisible = true;

    QWidget* m_sidebar = nullptr;
    QParallelAnimationGroup* m_sidebarAnimation = nullptr;
    QListView* m_list = nullptr;
    BufferListDelegate* m_delegate = nullptr;
    QStackedWidget* m_stack = nullptr;
    EditorView* m_editor = nullptr;
    PreviewView* m_preview = nullptr;
    OverlayBar* m_overlay = nullptr;
    Tray* m_tray = nullptr;
};
