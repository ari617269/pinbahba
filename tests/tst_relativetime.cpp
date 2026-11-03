#include <QTest>

#include "core/RelativeTime.h"

class RelativeTimeTest : public QObject {
    Q_OBJECT

private slots:
    void describe_data();
    void describe();
};

void RelativeTimeTest::describe_data() {
    const QDateTime now(QDate(2026, 3, 10), QTime(12, 0));

    QTest::addColumn<QDateTime>("then");
    QTest::addColumn<QString>("expected");

    QTest::newRow("seconds ago") << now.addSecs(-5) << QStringLiteral("just now");
    QTest::newRow("future") << now.addSecs(30) << QStringLiteral("just now");
    QTest::newRow("minutes") << now.addSecs(-5 * 60) << QStringLiteral("5m");
    QTest::newRow("hours") << now.addSecs(-3 * 3600) << QStringLiteral("3h");
    QTest::newRow("days") << now.addDays(-2) << QStringLiteral("2d");
    QTest::newRow("older than a week") << QDateTime(QDate(2026, 3, 1), QTime(9, 0))
                                       << QStringLiteral("Mar 1");
}

void RelativeTimeTest::describe() {
    QFETCH(QDateTime, then);
    QFETCH(QString, expected);
    const QDateTime now(QDate(2026, 3, 10), QTime(12, 0));
    QCOMPARE(RelativeTime::describe(then, now), expected);
}

QTEST_APPLESS_MAIN(RelativeTimeTest)
#include "tst_relativetime.moc"
