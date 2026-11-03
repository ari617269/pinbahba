#include <QTest>

#include "core/Title.h"

class TitleTest : public QObject {
    Q_OBJECT

private slots:
    void fromText_data();
    void fromText();
};

void TitleTest::fromText_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QStringLiteral("untitled");
    QTest::newRow("plain text") << QStringLiteral("just words") << QStringLiteral("untitled");
    QTest::newRow("heading") << QStringLiteral("# Hello") << QStringLiteral("Hello");
    QTest::newRow("leading blank lines") << QStringLiteral("\n\n  \n## Second")
                                         << QStringLiteral("Second");
    QTest::newRow("triple hash") << QStringLiteral("### Deep") << QStringLiteral("Deep");
    QTest::newRow("emphasis and code") << QStringLiteral("# *bold* and _under_ `code`")
                                       << QStringLiteral("bold and under code");
    QTest::newRow("bare hash") << QStringLiteral("#") << QStringLiteral("untitled");
    QTest::newRow("first heading wins") << QStringLiteral("intro\n# One\n# Two")
                                        << QStringLiteral("One");
    QTest::newRow("truncated at 60") << (QStringLiteral("# ") + QString(70, QLatin1Char('a')))
                                     << QString(60, QLatin1Char('a'));
}

void TitleTest::fromText() {
    QFETCH(QString, text);
    QFETCH(QString, expected);
    QCOMPARE(Title::fromText(text), expected);
}

QTEST_APPLESS_MAIN(TitleTest)
#include "tst_title.moc"
