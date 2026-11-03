#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTest>

#include "core/BufferStore.h"

namespace {

QString freshDir(const QString& name) {
    const QString path = QDir::current().filePath(QStringLiteral("test-data/") + name);
    QDir(path).removeRecursively();
    QDir().mkpath(path);
    return path;
}

QString fileFor(const QString& root, const QString& id) {
    return QDir(root).filePath(QStringLiteral("buffers/") + id + QStringLiteral(".md"));
}

void writeExternally(const QString& path, const QString& text) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(text.toUtf8());
}

}  // namespace

class BufferStoreTest : public QObject {
    Q_OBJECT

private slots:
    void roundTrip();
    void debouncesWrites();
    void pruneIfEmpty();
    void externalEditIsPickedUp();
    void pendingEditsWinOverExternalEdit();
    void editedBufferMovesToTop();
};

void BufferStoreTest::roundTrip() {
    const QString root = freshDir(QStringLiteral("roundTrip"));
    QString id;
    {
        BufferStore store(root);
        id = store.create();
        store.setText(id, QStringLiteral("# Hello\nbody"));
        store.flush();
    }

    BufferStore reopened(root);
    QCOMPARE(reopened.rowCount(), 1);
    QCOMPARE(reopened.text(id), QStringLiteral("# Hello\nbody"));
    QCOMPARE(reopened.index(0).data(BufferStore::TitleRole).toString(), QStringLiteral("Hello"));
}

void BufferStoreTest::debouncesWrites() {
    const QString root = freshDir(QStringLiteral("debounce"));
    BufferStore store(root);
    const QString id = store.create();

    QSignalSpy saved(&store, &BufferStore::bufferSaved);
    for (int i = 0; i < 10; ++i) {
        store.setText(id, QString::number(i));
        QTest::qWait(10);
    }
    QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 1000);
    QTest::qWait(400);
    QCOMPARE(saved.count(), 1);

    QFile file(fileFor(root, id));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("9"));
}

void BufferStoreTest::pruneIfEmpty() {
    const QString root = freshDir(QStringLiteral("prune"));
    BufferStore store(root);
    const QString empty = store.create();
    const QString full = store.create();
    store.setText(full, QStringLiteral("# keep"));
    store.flush();

    QVERIFY(!store.pruneIfEmpty(full));
    QVERIFY(store.pruneIfEmpty(empty));
    QVERIFY(!QFile::exists(fileFor(root, empty)));
    QVERIFY(QFile::exists(fileFor(root, full)));
    QCOMPARE(store.rowCount(), 1);
}

void BufferStoreTest::externalEditIsPickedUp() {
    const QString root = freshDir(QStringLiteral("external"));
    BufferStore store(root);
    const QString id = store.create();
    store.setText(id, QStringLiteral("# one"));
    store.flush();
    QTest::qWait(100);

    writeExternally(fileFor(root, id), QStringLiteral("# edited elsewhere"));
    QTRY_COMPARE_WITH_TIMEOUT(store.text(id), QStringLiteral("# edited elsewhere"), 1000);
}

void BufferStoreTest::pendingEditsWinOverExternalEdit() {
    const QString root = freshDir(QStringLiteral("pending"));
    BufferStore store(root);
    const QString id = store.create();
    store.setText(id, QStringLiteral("local"));

    writeExternally(fileFor(root, id), QStringLiteral("external"));
    QTest::qWait(100);
    QCOMPARE(store.text(id), QStringLiteral("local"));

    store.flush();
    QCOMPARE(store.text(id), QStringLiteral("local"));
}

void BufferStoreTest::editedBufferMovesToTop() {
    const QString root = freshDir(QStringLiteral("order"));
    BufferStore store(root);
    const QString first = store.create();
    QTest::qWait(5);
    const QString second = store.create();
    QCOMPARE(store.index(0).data(BufferStore::IdRole).toString(), second);

    QTest::qWait(5);
    store.setText(first, QStringLiteral("edited"));
    QCOMPARE(store.index(0).data(BufferStore::IdRole).toString(), first);
    QCOMPARE(store.index(1).data(BufferStore::IdRole).toString(), second);
}

QTEST_GUILESS_MAIN(BufferStoreTest)
#include "tst_bufferstore.moc"
