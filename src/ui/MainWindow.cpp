#include "ui/MainWindow.h"

#include <QAbstractAnimation>
#include <QAbstractItemView>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QListView>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyleHints>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/BufferStore.h"
#include "core/PdfExporter.h"
#include "core/Title.h"
#include "platform/Dock.h"
#include "platform/Reveal.h"
#include "platform/Tray.h"
#include "ui/AppMenu.h"
#include "ui/BufferListDelegate.h"
#include "ui/EditorView.h"
#include "ui/OverlayBar.h"
#include "ui/PreviewView.h"

namespace {

constexpr int kDefaultWidth = 1100;
constexpr int kDefaultHeight = 700;
constexpr int kSidebarWidth = 220;
constexpr int kSidebarAnimationMs = 180;
constexpr int kRelativeTimeRefreshMs = 60 * 1000;
constexpr int kOverlayInset = 16;
constexpr int kRecentBuffers = 5;

}  // namespace

MainWindow::MainWindow(BufferStore* store, std::optional<Mode> mode, ThemeChoice theme,
                       const QString& downloadsDir, QSettings* settings, QWidget* parent)
    : QMainWindow(parent),
      m_store(store),
      m_themeChoice(theme),
      m_downloadsDir(downloadsDir),
      m_settings(settings) {
    setWindowTitle(QStringLiteral("md"));
    resize(kDefaultWidth, kDefaultHeight);
    restoreGeometry(m_settings->value(QStringLiteral("window/geometry")).toByteArray());
    m_sidebarVisible = m_settings->value(QStringLiteral("window/sidebarVisible"), true).toBool();

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));
    setCentralWidget(central);

    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_sidebar = new QWidget(central);
    m_sidebar->setObjectName(QStringLiteral("sidebar"));
    m_sidebar->setFixedWidth(kSidebarWidth);

    auto* sideLayout = new QVBoxLayout(m_sidebar);
    sideLayout->setContentsMargins(14, 12, 14, 12);

    auto* newButton = new QToolButton(m_sidebar);
    newButton->setObjectName(QStringLiteral("newBuffer"));
    newButton->setText(QStringLiteral("+"));
    newButton->setAutoRaise(true);

    m_delegate = new BufferListDelegate(this);
    m_list = new QListView(m_sidebar);
    m_list->setObjectName(QStringLiteral("bufferList"));
    m_list->setModel(m_store);
    m_list->setItemDelegate(m_delegate);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMouseTracking(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    sideLayout->addWidget(newButton);
    sideLayout->addWidget(m_list, 1);

    m_stack = new QStackedWidget(central);
    m_stack->setObjectName(QStringLiteral("stack"));
    m_editor = new EditorView(m_stack);
    m_preview = new PreviewView(m_stack);
    m_stack->addWidget(m_editor);
    m_stack->addWidget(m_preview);

    root->addWidget(m_sidebar);
    root->addWidget(m_stack, 1);

    if (!m_sidebarVisible) {
        m_sidebar->setMinimumWidth(0);
        m_sidebar->setMaximumWidth(0);
        m_sidebar->hide();
    }

    m_overlay = new OverlayBar(central);

    auto* group = new QParallelAnimationGroup(this);
    const char* const widthProperties[] = {"minimumWidth", "maximumWidth"};
    for (const char* property : widthProperties) {
        auto* animation = new QPropertyAnimation(m_sidebar, property, group);
        animation->setDuration(kSidebarAnimationMs);
        group->addAnimation(animation);
    }
    m_sidebarAnimation = group;
    connect(group, &QAbstractAnimation::finished, this, [this] {
        if (!m_sidebarVisible) m_sidebar->hide();
    });

    connect(newButton, &QToolButton::clicked, this, &MainWindow::newBuffer);
    connect(m_list->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) { showBuffer(current); });
    connect(m_editor, &QTextEdit::textChanged, this, &MainWindow::onEditorTextChanged);
    connect(m_preview, &PreviewView::editRequested, this, [this] { setMode(Mode::Edit); });
    connect(m_overlay, &OverlayBar::editRequested, this, &MainWindow::toggleMode);
    connect(m_overlay, &OverlayBar::exportRequested, this, &MainWindow::exportPdf);
    connect(m_overlay, &OverlayBar::revealRequested, this, &MainWindow::revealLastExport);
    connect(m_overlay, &OverlayBar::contentChanged, this, &MainWindow::positionOverlay);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this] { applyTheme(); });

    auto* clock = new QTimer(this);
    clock->setInterval(kRelativeTimeRefreshMs);
    connect(clock, &QTimer::timeout, this, [this] { m_list->viewport()->update(); });
    clock->start();

    applyTheme();
    if (dockIconHidden()) Platform::setDockIconHidden(true);

    const QString lastBuffer = m_settings->value(QStringLiteral("session/lastBuffer")).toString();
    int row = 0;
    for (int candidate = 0; candidate < m_store->rowCount(); ++candidate) {
        if (m_store->index(candidate, 0).data(BufferStore::IdRole).toString() == lastBuffer) {
            row = candidate;
            break;
        }
    }
    if (m_store->rowCount() > 0) m_list->setCurrentIndex(m_store->index(row, 0));

    const bool storedEdit =
        m_settings->value(QStringLiteral("session/mode")).toString() == QLatin1String("edit");
    setMode(mode.value_or(storedEdit ? Mode::Edit : Mode::View));

    new AppMenu(this);
}

void MainWindow::setMode(Mode mode) {
    m_mode = mode;
    if (mode == Mode::View) {
        m_preview->showMarkdown(m_store->text(m_currentId));
        m_stack->setCurrentWidget(m_preview);
    } else {
        m_stack->setCurrentWidget(m_editor);
        m_editor->setFocus();
    }
    emit modeChanged(mode);
}

void MainWindow::toggleMode() {
    setMode(m_mode == Mode::Edit ? Mode::View : Mode::Edit);
}

void MainWindow::toggleSidebar() {
    m_sidebarVisible = !m_sidebarVisible;
    if (m_sidebarVisible) m_sidebar->show();

    const int target = m_sidebarVisible ? kSidebarWidth : 0;
    for (int i = 0; i < m_sidebarAnimation->animationCount(); ++i) {
        static_cast<QPropertyAnimation*>(m_sidebarAnimation->animationAt(i))->setEndValue(target);
    }
    m_sidebarAnimation->stop();
    m_sidebarAnimation->start();
}

void MainWindow::newBuffer() {
    if (!m_currentId.isEmpty() && m_store->text(m_currentId).trimmed().isEmpty()) {
        setMode(Mode::Edit);
        return;
    }
    m_store->create();
    m_list->setCurrentIndex(m_store->index(0, 0));
    setMode(Mode::Edit);
}

void MainWindow::deleteBuffer() {
    if (m_currentId.isEmpty()) return;

    if (!m_store->text(m_currentId).trimmed().isEmpty()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, QStringLiteral("Delete buffer"), QStringLiteral("Delete this buffer?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }
    m_store->remove(m_currentId);
}

void MainWindow::exportPdf() {
    if (m_currentId.isEmpty()) return;

    const QString text = m_store->text(m_currentId);
    const QString title = Title::fromText(text);
    const QString path = PdfExporter::downloadsPath(title, m_downloadsDir);
    if (!PdfExporter::exportTo(text, title, path)) {
        QMessageBox::warning(this, QStringLiteral("Export failed"),
                             QStringLiteral("The PDF could not be written to Downloads."));
        return;
    }

    m_lastExport = path;
    m_overlay->showSaved();
    positionOverlay();
}

void MainWindow::revealLastExport() {
    if (m_lastExport.isEmpty()) return;
    Platform::revealInFileManager(m_lastExport);
}

void MainWindow::revealBuffersFolder() {
    Platform::revealInFileManager(m_store->directory());
}

void MainWindow::selectRelative(int delta) {
    const int row = m_list->currentIndex().row() + delta;
    if (row >= 0 && row < m_store->rowCount()) m_list->setCurrentIndex(m_store->index(row, 0));
}

void MainWindow::openBuffer(const QString& id) {
    for (int row = 0; row < m_store->rowCount(); ++row) {
        const QModelIndex index = m_store->index(row, 0);
        if (index.data(BufferStore::IdRole).toString() != id) continue;
        m_list->setCurrentIndex(index);
        setMode(Mode::View);
        return;
    }
}

void MainWindow::showWindow() {
    show();
    raise();
    activateWindow();
}

void MainWindow::toggleWindow() {
    if (isVisible()) {
        hide();
    } else {
        showWindow();
    }
}

bool MainWindow::enableTray() {
    if (!Tray::isAvailable()) return false;

    m_tray = new Tray(this);
    connect(m_tray, &Tray::menuAboutToShow, this, &MainWindow::refreshTrayRecents);
    connect(m_tray, &Tray::newBufferRequested, this, [this] {
        showWindow();
        newBuffer();
    });
    connect(m_tray, &Tray::toggleWindowRequested, this, &MainWindow::toggleWindow);
    connect(m_tray, &Tray::openRequested, this, [this](const QString& id) {
        showWindow();
        openBuffer(id);
    });
    connect(m_tray, &Tray::quitRequested, this, [] { QCoreApplication::quit(); });

    m_tray->show();
    m_tray->setWindowShown(isVisible());
    return true;
}

void MainWindow::saveState() {
    m_settings->setValue(QStringLiteral("window/geometry"), saveGeometry());
    m_settings->setValue(QStringLiteral("window/sidebarVisible"), m_sidebarVisible);
    m_settings->setValue(QStringLiteral("session/mode"),
                         m_mode == Mode::Edit ? QStringLiteral("edit") : QStringLiteral("view"));
    m_settings->setValue(QStringLiteral("session/lastBuffer"), m_currentId);
    m_settings->sync();
}

bool MainWindow::dockIconHidden() const {
    return m_settings->value(QStringLiteral("window/hideDockIcon"), false).toBool();
}

void MainWindow::setDockIconHidden(bool hidden) {
    Platform::setDockIconHidden(hidden);
    m_settings->setValue(QStringLiteral("window/hideDockIcon"), hidden);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    positionOverlay();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_tray && m_tray->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    saveState();
    m_store->flush();
    event->accept();
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (m_tray) m_tray->setWindowShown(true);
}

void MainWindow::hideEvent(QHideEvent* event) {
    QMainWindow::hideEvent(event);
    if (m_tray) m_tray->setWindowShown(false);
}

void MainWindow::positionOverlay() {
    m_overlay->adjustSize();
    m_overlay->move(centralWidget()->width() - m_overlay->width() - kOverlayInset, kOverlayInset);
    m_overlay->raise();
}

void MainWindow::refreshTrayRecents() {
    QList<Tray::Entry> entries;
    for (int row = 0; row < m_store->rowCount() && row < kRecentBuffers; ++row) {
        const QModelIndex index = m_store->index(row, 0);
        entries.append(Tray::Entry{index.data(BufferStore::IdRole).toString(),
                                   index.data(BufferStore::TitleRole).toString()});
    }
    m_tray->setRecent(entries);
}

Theme::Palette MainWindow::resolvedPalette() const {
    bool dark = m_themeChoice == ThemeChoice::Dark;
    if (m_themeChoice == ThemeChoice::System) {
        dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    }
    return dark ? Theme::dark() : Theme::light();
}

void MainWindow::applyTheme() {
    const Theme::Palette palette = resolvedPalette();
    m_delegate->applyPalette(palette);
    m_editor->applyPalette(palette);
    m_preview->applyPalette(palette);
    m_overlay->applyPalette(palette);

    setStyleSheet(QStringLiteral(
                      "QWidget#central, QWidget#sidebar, QListView { background-color: %1; "
                      "border: none; }"
                      "QToolButton#newBuffer { color: %2; border: none; font-size: 18px; }")
                      .arg(palette.background.name(), palette.muted.name()));
    m_list->viewport()->update();
}

void MainWindow::showBuffer(const QModelIndex& index) {
    const QString previous = m_currentId;
    m_currentId = index.isValid() ? index.data(BufferStore::IdRole).toString() : QString();
    if (!previous.isEmpty() && previous != m_currentId) {
        QTimer::singleShot(0, this, [this, previous] {
            if (previous != m_currentId) m_store->pruneIfEmpty(previous);
        });
    }

    const QString text = m_store->text(m_currentId);
    m_editor->setText(text);
    m_preview->showMarkdown(text);
}

void MainWindow::onEditorTextChanged() {
    if (m_currentId.isEmpty()) return;
    m_store->setText(m_currentId, m_editor->toPlainText());
}
