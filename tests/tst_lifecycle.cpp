#include <QDir>
#include <QTest>

#include "core/BufferStore.h"

namespace {

QString freshDir(const QString& name) {
    const QString path = QDir::current().filePath(QStringLiteral("test-data/") + name);
    QDir(path).removeRecursively();
    QDir().mkpath(path);
    return path;
}

}  // namespace

class LifecycleTest : public QObject {
    Q_OBJECT

private slots:
    void quitWithinDebounceKeepsEdits();
};

void LifecycleTest::quitWithinDebounceKeepsEdits() {
    const QString root = freshDir(QStringLiteral("lifecycle"));
    QString id;
    {
        BufferStore store(root);
        id = store.create();
        store.setText(id, QStringLiteral("typed just now"));
        QTest::qWait(100);
        store.flush();
    }

    BufferStore reopened(root);
    QCOMPARE(reopened.text(id), QStringLiteral("typed just now"));
}

QTEST_GUILESS_MAIN(LifecycleTest)
#include "tst_lifecycle.moc"
