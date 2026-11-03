#pragma once

#include <QList>
#include <QObject>
#include <QString>

class QAction;
class QMenu;
class QSystemTrayIcon;

class Tray : public QObject {
    Q_OBJECT

public:
    struct Entry {
        QString id;
        QString title;
    };

    explicit Tray(QObject* parent = nullptr);
    ~Tray() override;

    static bool isAvailable();

    void show();
    bool isVisible() const;
    void setRecent(const QList<Entry>& entries);
    void setWindowShown(bool shown);

signals:
    void menuAboutToShow();
    void newBufferRequested();
    void toggleWindowRequested();
    void openRequested(const QString& id);
    void quitRequested();

private:
    QSystemTrayIcon* m_icon = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_recentAnchor = nullptr;
    QAction* m_toggle = nullptr;
    QList<QAction*> m_recent;
};
