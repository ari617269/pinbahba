#include <QTest>

#include "core/Markdown.h"

class MarkdownTest : public QObject {
    Q_OBJECT

private slots:
    void headings();
    void gfmTable();
    void fencedCode();
    void escapesRawHtml();
};

void MarkdownTest::headings() {
    QVERIFY(Markdown::toHtml(QStringLiteral("# Hello")).contains(QStringLiteral("<h1>Hello</h1>")));
}

void MarkdownTest::gfmTable() {
    const QString html = Markdown::toHtml(
        QStringLiteral("| a | b |\n|---|---|\n| 1 | 2 |\n"));
    QVERIFY(html.contains(QStringLiteral("<table>")));
    QVERIFY(html.contains(QStringLiteral("<td>1</td>")));
}

void MarkdownTest::fencedCode() {
    const QString html = Markdown::toHtml(QStringLiteral("```\nint x;\n```\n"));
    QVERIFY(html.contains(QStringLiteral("<pre><code>int x;")));
}

void MarkdownTest::escapesRawHtml() {
    const QString html = Markdown::toHtml(QStringLiteral("<script>alert(1)</script>"));
    QVERIFY(!html.contains(QStringLiteral("<script>")));
    QVERIFY(html.contains(QStringLiteral("&lt;script&gt;")));
}

QTEST_GUILESS_MAIN(MarkdownTest)
#include "tst_markdown.moc"
