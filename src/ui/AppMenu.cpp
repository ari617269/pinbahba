#include "ui/AppMenu.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextEdit>

#include "ui/MainWindow.h"

namespace {

template <typename Slot>
QAction* addAction(QMenu* menu, const QString& text, const QKeySequence& shortcut,
                   QObject* receiver, Slot slot) {
    QAction* action = menu->addAction(text);
    action->setShortcut(shortcut);
    QObject::connect(action, &QAction::triggered, receiver, slot);
    return action;
}

void forwardToFocusedEditor(void (QTextEdit::*operation)()) {
    if (auto* editor = qobject_cast<QTextEdit*>(QApplication::focusWidget())) {
        (editor->*operation)();
    }
}

}  // namespace

AppMenu::AppMenu(MainWindow* window) : QObject(window) {
    QMenuBar* bar = window->menuBar();

    QMenu* file = bar->addMenu(QStringLiteral("File"));
    addAction(file, QStringLiteral("New Buffer"), QKeySequence(Qt::CTRL | Qt::Key_N), window,
              [window] { window->newBuffer(); });
    addAction(file, QStringLiteral("Export PDF"), QKeySequence(Qt::CTRL | Qt::Key_P), window,
              [window] { window->exportPdf(); });
    addAction(file, QStringLiteral("Delete Buffer"), QKeySequence(Qt::CTRL | Qt::Key_Backspace),
              window, [window] { window->deleteBuffer(); });
    addAction(file, QStringLiteral("Reveal Buffers Folder"), QKeySequence(), window,
              [window] { window->revealBuffersFolder(); });
    file->addSeparator();
    QAction* quit = addAction(file, QStringLiteral("Quit"), QKeySequence::Quit, window,
                              [] { QApplication::quit(); });
    quit->setMenuRole(QAction::QuitRole);

    QMenu* edit = bar->addMenu(QStringLiteral("Edit"));
    addAction(edit, QStringLiteral("Undo"), QKeySequence::Undo, window,
              [] { forwardToFocusedEditor(&QTextEdit::undo); });
    addAction(edit, QStringLiteral("Redo"), QKeySequence::Redo, window,
              [] { forwardToFocusedEditor(&QTextEdit::redo); });
    edit->addSeparator();
    addAction(edit, QStringLiteral("Cut"), QKeySequence::Cut, window,
              [] { forwardToFocusedEditor(&QTextEdit::cut); });
    addAction(edit, QStringLiteral("Copy"), QKeySequence::Copy, window,
              [] { forwardToFocusedEditor(&QTextEdit::copy); });
    addAction(edit, QStringLiteral("Paste"), QKeySequence::Paste, window,
              [] { forwardToFocusedEditor(&QTextEdit::paste); });
    addAction(edit, QStringLiteral("Select All"), QKeySequence::SelectAll, window,
              [] { forwardToFocusedEditor(&QTextEdit::selectAll); });

    QMenu* view = bar->addMenu(QStringLiteral("View"));
    addAction(view, QStringLiteral("Toggle Edit/Preview"), QKeySequence(Qt::CTRL | Qt::Key_E),
              window, [window] { window->toggleMode(); });
    addAction(view, QStringLiteral("Toggle Sidebar"), QKeySequence(Qt::CTRL | Qt::Key_Backslash),
              window, [window] { window->toggleSidebar(); });
#ifdef Q_OS_MACOS
    QAction* hideDock = view->addAction(QStringLiteral("Hide Dock Icon"));
    hideDock->setCheckable(true);
    hideDock->setChecked(window->dockIconHidden());
    QObject::connect(hideDock, &QAction::toggled, window,
                     [window](bool hidden) { window->setDockIconHidden(hidden); });
#endif

    QMenu* go = bar->addMenu(QStringLiteral("Go"));
    QAction* previous = addAction(go, QStringLiteral("Previous"),
                                  QKeySequence(Qt::CTRL | Qt::Key_Up), window,
                                  [window] { window->selectRelative(-1); });
    QAction* next = addAction(go, QStringLiteral("Next"), QKeySequence(Qt::CTRL | Qt::Key_Down),
                              window, [window] { window->selectRelative(1); });
    previous->setEnabled(window->mode() == MainWindow::Mode::View);
    next->setEnabled(window->mode() == MainWindow::Mode::View);
    QObject::connect(window, &MainWindow::modeChanged, this,
                     [previous, next](MainWindow::Mode mode) {
                         previous->setEnabled(mode == MainWindow::Mode::View);
                         next->setEnabled(mode == MainWindow::Mode::View);
                     });

    QMenu* windowMenu = bar->addMenu(QStringLiteral("Window"));
    addAction(windowMenu, QStringLiteral("Minimize"), QKeySequence(Qt::CTRL | Qt::Key_M), window,
              [window] { window->showMinimized(); });
#ifdef Q_OS_MACOS
    addAction(windowMenu, QStringLiteral("Zoom"), QKeySequence(), window, [window] {
        window->isMaximized() ? window->showNormal() : window->showMaximized();
    });
#endif

    QMenu* help = bar->addMenu(QStringLiteral("Help"));
    QAction* about = addAction(help, QStringLiteral("About md"), QKeySequence(), window, [window] {
        QMessageBox::about(window, QStringLiteral("About md"),
                           QStringLiteral("md: ultraminimal markdown buffers."));
    });
    about->setMenuRole(QAction::AboutRole);
}
