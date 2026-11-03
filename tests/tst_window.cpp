#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QListView>
#include <QSettings>
#include <QStackedWidget>
#include <QTest>
#include <QTextBrowser>
#include <QTextEdit>

#include "core/BufferStore.h"
#include "ui/MainWindow.h"
#include "ui/OverlayBar.h"

namespace {

QString freshDir(const QString& name) {
    const QString path = QDir::current().filePath(QStringLiteral("test-data/") + name);
    QDir(path).removeRecursively();
    QDir().mkpath(path);
    return path;
}

QString settingsPathFor(const QString& name) {
    return freshDir(name) + QStringLiteral("/settings.ini");
}

QModelIndex rowForId(const BufferStore& store, const QString& id) {
    for (int row = 0; row < store.rowCount(); ++row) {
        const QModelIndex index = store.index(row, 0);
        if (index.data(BufferStore::IdRole).toString() == id) return index;
    }
    return {};
}

QAction* actionNamed(const MainWindow& window, const QString& text) {
    for (QAction* action : window.findChildren<QAction*>()) {
        if (action->text() == text) return action;
    }
    return nullptr;
}

}  // namespace

class WindowTest : public QObject {
    Q_OBJECT

private slots:
    void windowTitle();
    void sidebarWidth();
    void panesHaveNoFrame();
    void overlayFadesOnHover();
    void modeSwitchesPane();
    void ctrlETogglesMode();
    void newBufferOpensEdit();
    void plusReusesEmptyBuffer();
    void pruneOnSwitch();
    void exportWritesPdf();
    void tabInsertsSpaces();
    void deleteEmptyBuffer();
    void goActionsOnlyInView();
    void sidebarCollapses();
    void closeWithoutTrayFlushesEdits();
    void restoresLastBuffer();

private:
    static void showAndWait(MainWindow& window);
};

void WindowTest::showAndWait(MainWindow& window) {
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
}

void WindowTest::windowTitle() {
    BufferStore store(freshDir(QStringLiteral("window-title")));
    QSettings settings(settingsPathFor(QStringLiteral("window-title")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-title-downloads")), &settings);
    QCOMPARE(window.windowTitle(), QStringLiteral("md"));
}

void WindowTest::sidebarWidth() {
    BufferStore store(freshDir(QStringLiteral("window-sidebar")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-sidebar")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-sidebar-downloads")), &settings);
    showAndWait(window);
    QCOMPARE(window.findChild<QWidget*>(QStringLiteral("sidebar"))->width(), 220);
}

void WindowTest::panesHaveNoFrame() {
    BufferStore store(freshDir(QStringLiteral("window-frames")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-frames")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-frames-downloads")), &settings);
    showAndWait(window);

    auto* editor = window.findChild<QTextEdit*>(QStringLiteral("editor"));
    auto* preview = window.findChild<QTextBrowser*>(QStringLiteral("preview"));
    QVERIFY(editor->frameShape() == QFrame::NoFrame);
    QVERIFY(preview->frameShape() == QFrame::NoFrame);
}

void WindowTest::overlayFadesOnHover() {
    BufferStore store(freshDir(QStringLiteral("window-overlay")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-overlay")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-overlay-downloads")), &settings);
    showAndWait(window);

    auto* overlay = window.findChild<OverlayBar*>(QStringLiteral("overlay"));
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(overlay->graphicsEffect());
    QVERIFY(effect);
    QCOMPARE(effect->opacity(), 0.25);

    QEnterEvent enter{QPointF(), QPointF(), QPointF()};
    QApplication::sendEvent(overlay, &enter);
    QTRY_COMPARE_WITH_TIMEOUT(effect->opacity(), 1.0, 1000);

    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(overlay, &leave);
    QTRY_COMPARE_WITH_TIMEOUT(effect->opacity(), 0.25, 1000);
}

void WindowTest::modeSwitchesPane() {
    BufferStore store(freshDir(QStringLiteral("window-mode")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-mode")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-mode-downloads")), &settings);
    showAndWait(window);

    auto* stack = window.findChild<QStackedWidget*>(QStringLiteral("stack"));
    QVERIFY(stack->currentWidget() == window.findChild<QWidget*>(QStringLiteral("editor")));

    window.setMode(MainWindow::Mode::View);
    QVERIFY(window.mode() == MainWindow::Mode::View);
    QVERIFY(stack->currentWidget() == window.findChild<QWidget*>(QStringLiteral("preview")));
}

void WindowTest::ctrlETogglesMode() {
    BufferStore store(freshDir(QStringLiteral("window-ctrl-e")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-ctrl-e")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::View, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-ctrl-e-downloads")), &settings);
    showAndWait(window);

    QTest::keyClick(&window, Qt::Key_E, Qt::ControlModifier);
    QVERIFY(window.mode() == MainWindow::Mode::Edit);
    QTest::keyClick(&window, Qt::Key_E, Qt::ControlModifier);
    QVERIFY(window.mode() == MainWindow::Mode::View);
}

void WindowTest::newBufferOpensEdit() {
    BufferStore store(freshDir(QStringLiteral("window-new")));
    const QString id = store.create();
    store.setText(id, QStringLiteral("# keep"));
    store.flush();
    QSettings settings(settingsPathFor(QStringLiteral("window-new")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::View, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-new-downloads")), &settings);

    window.newBuffer();
    QCOMPARE(store.rowCount(), 2);
    QVERIFY(window.mode() == MainWindow::Mode::Edit);
}

void WindowTest::plusReusesEmptyBuffer() {
    BufferStore store(freshDir(QStringLiteral("window-reuse")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-reuse")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-reuse-downloads")), &settings);

    window.newBuffer();
    QCOMPARE(store.rowCount(), 1);
}

void WindowTest::pruneOnSwitch() {
    BufferStore store(freshDir(QStringLiteral("window-prune")));
    const QString kept = store.create();
    store.setText(kept, QStringLiteral("# kept"));
    store.flush();
    QSettings settings(settingsPathFor(QStringLiteral("window-prune")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::View, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-prune-downloads")), &settings);

    window.newBuffer();
    QCOMPARE(store.rowCount(), 2);

    window.findChild<QListView*>(QStringLiteral("bufferList"))
        ->setCurrentIndex(rowForId(store, kept));
    QTRY_COMPARE_WITH_TIMEOUT(store.rowCount(), 1, 1000);
}

void WindowTest::exportWritesPdf() {
    BufferStore store(freshDir(QStringLiteral("window-export")));
    const QString id = store.create();
    store.setText(id, QStringLiteral("# Export me"));
    store.flush();
    const QString downloads = freshDir(QStringLiteral("window-export-downloads"));
    QSettings settings(settingsPathFor(QStringLiteral("window-export")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::View, MainWindow::ThemeChoice::Light, downloads,
                      &settings);

    window.exportPdf();
    QVERIFY(QFile::exists(QDir(downloads).filePath(QStringLiteral("Export me.pdf"))));
}

void WindowTest::tabInsertsSpaces() {
    BufferStore store(freshDir(QStringLiteral("window-tab")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-tab")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-tab-downloads")), &settings);
    showAndWait(window);

    auto* editor = window.findChild<QTextEdit*>(QStringLiteral("editor"));
    QTest::keyClick(editor, Qt::Key_Tab);
    QCOMPARE(editor->toPlainText(), QStringLiteral("  "));
}

void WindowTest::deleteEmptyBuffer() {
    BufferStore store(freshDir(QStringLiteral("window-delete")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-delete")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-delete-downloads")), &settings);

    window.deleteBuffer();
    QCOMPARE(store.rowCount(), 0);
}

void WindowTest::goActionsOnlyInView() {
    BufferStore store(freshDir(QStringLiteral("window-go")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-go")), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::View, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-go-downloads")), &settings);

    QAction* previous = actionNamed(window, QStringLiteral("Previous"));
    QVERIFY(previous);
    QVERIFY(previous->isEnabled());

    window.setMode(MainWindow::Mode::Edit);
    QVERIFY(!previous->isEnabled());
}

void WindowTest::sidebarCollapses() {
    BufferStore store(freshDir(QStringLiteral("window-sidebar-toggle")));
    store.create();
    QSettings settings(settingsPathFor(QStringLiteral("window-sidebar-toggle")),
                       QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      freshDir(QStringLiteral("window-sidebar-toggle-downloads")), &settings);
    showAndWait(window);

    auto* sidebar = window.findChild<QWidget*>(QStringLiteral("sidebar"));
    window.toggleSidebar();
    QTRY_VERIFY_WITH_TIMEOUT(sidebar->isHidden(), 1000);
}

void WindowTest::closeWithoutTrayFlushesEdits() {
    QApplication::setQuitOnLastWindowClosed(false);
    const QString root = freshDir(QStringLiteral("window-close"));
    BufferStore store(root);
    const QString id = store.create();
    QSettings settings(root + QStringLiteral("/settings.ini"), QSettings::IniFormat);
    MainWindow window(&store, MainWindow::Mode::Edit, MainWindow::ThemeChoice::Light,
                      root + QStringLiteral("/downloads"), &settings);
    showAndWait(window);

    store.setText(id, QStringLiteral("pending edit"));
    window.close();
    QVERIFY(!window.isVisible());

    BufferStore reopened(root);
    QCOMPARE(reopened.text(id), QStringLiteral("pending edit"));
}

void WindowTest::restoresLastBuffer() {
    const QString root = freshDir(QStringLiteral("window-restore"));
    BufferStore store(root);
    const QString first = store.create();
    store.setText(first, QStringLiteral("# first"));
    const QString second = store.create();
    store.setText(second, QStringLiteral("# second"));
    store.flush();
    QSettings settings(root + QStringLiteral("/settings.ini"), QSettings::IniFormat);

    {
        MainWindow window(&store, MainWindow::Mode::View, MainWindow::ThemeChoice::Light,
                          root + QStringLiteral("/downloads"), &settings);
        window.findChild<QListView*>(QStringLiteral("bufferList"))
            ->setCurrentIndex(rowForId(store, first));
        window.saveState();
    }

    MainWindow reopened(&store, std::nullopt, MainWindow::ThemeChoice::Light,
                        root + QStringLiteral("/downloads"), &settings);
    QCOMPARE(reopened.findChild<QListView*>(QStringLiteral("bufferList"))
                 ->currentIndex()
                 .data(BufferStore::IdRole)
                 .toString(),
             first);
}

QTEST_MAIN(WindowTest)
#include "tst_window.moc"
