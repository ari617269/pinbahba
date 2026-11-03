#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "platform/SingleInstance.h"

class SingleInstanceTest : public QObject {
    Q_OBJECT

private slots:
    void firstClaimWins();
    void secondLaunchNotifiesFirst();
};

void SingleInstanceTest::firstClaimWins() {
    SingleInstance first;
    QVERIFY(first.claim(QStringLiteral("md-test-first-%1").arg(QCoreApplication::applicationPid())));
}

void SingleInstanceTest::secondLaunchNotifiesFirst() {
    const QString name = QStringLiteral("md-test-second-%1").arg(QCoreApplication::applicationPid());

    SingleInstance first;
    QVERIFY(first.claim(name));
    QSignalSpy shown(&first, &SingleInstance::showRequested);

    SingleInstance second;
    QVERIFY(!second.claim(name));
    QTRY_COMPARE_WITH_TIMEOUT(shown.count(), 1, 2000);
}

QTEST_GUILESS_MAIN(SingleInstanceTest)
#include "tst_singleinstance.moc"
