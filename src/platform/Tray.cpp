#include "platform/Tray.h"

#include <utility>

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

namespace {

QIcon trayIcon() {
    QIcon icon;
#ifdef Q_OS_MACOS
    icon.addFile(QStringLiteral(":/icons/tray-mac.png"));
    icon.addFile(QStringLiteral(":/icons/tray-mac@2x.png"));
    icon.setIsMask(true);
#else
    icon = QIcon(QStringLiteral(":/icons/tray-linux.png"));
#endif
    return icon;
}

}  // namespace

Tray::Tray(QObject* parent)
    : QObject(parent), m_icon(new QSystemTrayIcon(this)), m_menu(new QMenu) {
    m_icon->setIcon(trayIcon());
    m_icon->setToolTip(QStringLiteral("md"));

    QAction* newBuffer = m_menu->addAction(QStringLiteral("New Buffer"));
    connect(newBuffer, &QAction::triggered, this, &Tray::newBufferRequested);

    m_recentAnchor = m_menu->addSeparator();

    m_toggle = m_menu->addAction(QStringLiteral("Show md"));
    connect(m_toggle, &QAction::triggered, this, &Tray::toggleWindowRequested);

    m_menu->addSeparator();
    QAction* quit = m_menu->addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, this, &Tray::quitRequested);

    m_icon->setContextMenu(m_menu);
    connect(m_menu, &QMenu::aboutToShow, this, &Tray::menuAboutToShow);

#ifndef Q_OS_MACOS
    connect(m_icon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) emit toggleWindowRequested();
            });
#endif
}

Tray::~Tray() {
    delete m_menu;
}

bool Tray::isAvailable() {
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void Tray::show() {
    m_icon->show();
}

bool Tray::isVisible() const {
    return m_icon->isVisible();
}

void Tray::setRecent(const QList<Entry>& entries) {
    for (QAction* action : std::as_const(m_recent)) {
        m_menu->removeAction(action);
        delete action;
    }
    m_recent.clear();

    for (const Entry& entry : entries) {
        QAction* action = new QAction(entry.title, m_menu);
        connect(action, &QAction::triggered, this, [this, id = entry.id] { emit openRequested(id); });
        m_menu->insertAction(m_recentAnchor, action);
        m_recent.append(action);
    }
}

void Tray::setWindowShown(bool shown) {
    m_toggle->setText(shown ? QStringLiteral("Hide md") : QStringLiteral("Show md"));
}
