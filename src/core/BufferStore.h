#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QString>

#include "core/Buffer.h"

class QTimer;

class BufferStore : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { IdRole = Qt::UserRole + 1, TextRole, TitleRole, UpdatedRole };

    explicit BufferStore(const QString& dataDir, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QString create();
    QString text(const QString& id) const;
    void setText(const QString& id, const QString& text);
    void remove(const QString& id);
    bool pruneIfEmpty(const QString& id);
    void flush();
    QString directory() const { return m_dir; }

signals:
    void bufferSaved(const QString& id);

private:
    int indexOf(const QString& id) const;
    QString pathFor(const QString& id) const;
    bool saveNow(const QString& id);
    void scheduleSave(const QString& id);
    int reposition(int row);
    void onFileChanged(const QString& path);

    QString m_dir;
    QList<Buffer> m_buffers;
    QHash<QString, QTimer*> m_timers;
    QFileSystemWatcher m_watcher;
};
