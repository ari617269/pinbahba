#include "core/BufferStore.h"

#include <algorithm>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTimer>
#include <QUuid>

#include "core/Title.h"

namespace {

constexpr int kDebounceMs = 300;

QString readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll());
}

}  // namespace

BufferStore::BufferStore(const QString& dataDir, QObject* parent)
    : QAbstractListModel(parent), m_dir(QDir(dataDir).filePath(QStringLiteral("buffers"))) {
    QDir().mkpath(m_dir);

    const QFileInfoList files = QDir(m_dir).entryInfoList({QStringLiteral("*.md")}, QDir::Files);
    for (const QFileInfo& info : files) {
        m_buffers.append(Buffer{info.completeBaseName(), readFile(info.filePath()), info.lastModified()});
        m_watcher.addPath(info.filePath());
    }
    std::stable_sort(m_buffers.begin(), m_buffers.end(),
                     [](const Buffer& a, const Buffer& b) { return a.updated > b.updated; });

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &BufferStore::onFileChanged);
}

int BufferStore::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_buffers.size();
}

QVariant BufferStore::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_buffers.size()) return {};

    const Buffer& buffer = m_buffers.at(index.row());
    switch (role) {
    case IdRole:
        return buffer.id;
    case TextRole:
        return buffer.text;
    case TitleRole:
        return Title::fromText(buffer.text);
    case UpdatedRole:
        return buffer.updated;
    default:
        return {};
    }
}

QString BufferStore::create() {
    const Buffer buffer{QUuid::createUuid().toString(QUuid::WithoutBraces), QString(),
                        QDateTime::currentDateTime()};
    beginInsertRows(QModelIndex(), 0, 0);
    m_buffers.prepend(buffer);
    endInsertRows();
    saveNow(buffer.id);
    return buffer.id;
}

QString BufferStore::text(const QString& id) const {
    const int row = indexOf(id);
    return row < 0 ? QString() : m_buffers.at(row).text;
}

void BufferStore::setText(const QString& id, const QString& text) {
    const int row = indexOf(id);
    if (row < 0 || m_buffers.at(row).text == text) return;

    Buffer& buffer = m_buffers[row];
    buffer.text = text;
    buffer.updated = QDateTime::currentDateTime();
    const int newRow = reposition(row);
    emit dataChanged(index(newRow), index(newRow));
    scheduleSave(id);
}

void BufferStore::remove(const QString& id) {
    const int row = indexOf(id);
    if (row < 0) return;

    delete m_timers.take(id);
    const QString path = pathFor(id);
    m_watcher.removePath(path);
    QFile::remove(path);

    beginRemoveRows(QModelIndex(), row, row);
    m_buffers.removeAt(row);
    endRemoveRows();
}

bool BufferStore::pruneIfEmpty(const QString& id) {
    const int row = indexOf(id);
    if (row < 0 || !m_buffers.at(row).text.trimmed().isEmpty()) return false;
    remove(id);
    return true;
}

void BufferStore::flush() {
    for (const QString& id : m_timers.keys()) {
        QTimer* timer = m_timers.value(id);
        if (timer->isActive()) {
            timer->stop();
            saveNow(id);
        }
    }
}

int BufferStore::indexOf(const QString& id) const {
    for (int i = 0; i < m_buffers.size(); ++i) {
        if (m_buffers.at(i).id == id) return i;
    }
    return -1;
}

QString BufferStore::pathFor(const QString& id) const {
    return QDir(m_dir).filePath(id + QStringLiteral(".md"));
}

bool BufferStore::saveNow(const QString& id) {
    const int row = indexOf(id);
    if (row < 0) return false;

    const QString path = pathFor(id);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(m_buffers.at(row).text.toUtf8()) < 0 ||
        !file.commit()) {
        qWarning() << "md: failed to save buffer" << id << file.errorString();
        return false;
    }

    m_watcher.removePath(path);
    m_watcher.addPath(path);
    emit bufferSaved(id);
    return true;
}

void BufferStore::scheduleSave(const QString& id) {
    QTimer*& timer = m_timers[id];
    if (!timer) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(kDebounceMs);
        connect(timer, &QTimer::timeout, this, [this, id] { saveNow(id); });
    }
    timer->start();
}

int BufferStore::reposition(int row) {
    const QDateTime updated = m_buffers.at(row).updated;
    int target = 0;
    for (int i = 0; i < m_buffers.size(); ++i) {
        if (i != row && m_buffers.at(i).updated > updated) ++target;
    }
    if (target == row) return row;

    const int destChild = target < row ? target : target + 1;
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), destChild);
    m_buffers.move(row, target);
    endMoveRows();
    return target;
}

void BufferStore::onFileChanged(const QString& path) {
    const QFileInfo info(path);
    if (!info.exists()) return;
    if (!m_watcher.files().contains(path)) m_watcher.addPath(path);

    const QString id = info.completeBaseName();
    const int row = indexOf(id);
    if (row < 0) return;

    const QTimer* timer = m_timers.value(id);
    if (timer && timer->isActive()) return;

    const QString disk = readFile(path);
    Buffer& buffer = m_buffers[row];
    if (disk == buffer.text) return;

    buffer.text = disk;
    buffer.updated = info.lastModified();
    const int newRow = reposition(row);
    emit dataChanged(index(newRow), index(newRow));
}
