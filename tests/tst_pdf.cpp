#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTest>

#include "core/PdfExporter.h"

namespace {

QString freshDir(const QString& name) {
    const QString path = QDir::current().filePath(QStringLiteral("test-data/") + name);
    QDir(path).removeRecursively();
    QDir().mkpath(path);
    return path;
}

QByteArray readAll(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

}  // namespace

class PdfTest : public QObject {
    Q_OBJECT

private slots:
    void writesPdf();
    void longDocumentPaginates();
    void dedupesFilenames();
};

void PdfTest::writesPdf() {
    const QString path = QDir(freshDir(QStringLiteral("pdf-basic"))).filePath(QStringLiteral("basic.pdf"));
    QVERIFY(PdfExporter::exportTo(QStringLiteral("# Title\n\nSome *text*."),
                                  QStringLiteral("Title"), path));

    const QByteArray data = readAll(path);
    QVERIFY(data.startsWith("%PDF"));
    QVERIFY(data.size() > 1024);
    QVERIFY(data.contains("/Title"));
}

void PdfTest::longDocumentPaginates() {
    const QString path = QDir(freshDir(QStringLiteral("pdf-long"))).filePath(QStringLiteral("long.pdf"));
    QString markdown;
    for (int i = 1; i <= 200; ++i) {
        markdown += QStringLiteral("Paragraph %1 with enough words to fill a line of body text.\n\n").arg(i);
    }
    QVERIFY(PdfExporter::exportTo(markdown, QStringLiteral("Long"), path));

    const QString text = QString::fromLatin1(readAll(path));
    const QRegularExpression pageObject(QStringLiteral("/Type\\s*/Page(?!s)"));
    QRegularExpressionMatchIterator it = pageObject.globalMatch(text);
    int pages = 0;
    while (it.hasNext()) {
        it.next();
        ++pages;
    }
    QVERIFY(pages > 1);
}

void PdfTest::dedupesFilenames() {
    const QString dir = freshDir(QStringLiteral("pdf-dedupe"));
    QCOMPARE(PdfExporter::downloadsPath(QStringLiteral("Title"), dir),
             QDir(dir).filePath(QStringLiteral("Title.pdf")));

    QFile first(QDir(dir).filePath(QStringLiteral("Title.pdf")));
    QVERIFY(first.open(QIODevice::WriteOnly));
    first.close();
    QCOMPARE(PdfExporter::downloadsPath(QStringLiteral("Title"), dir),
             QDir(dir).filePath(QStringLiteral("Title (2).pdf")));

    QFile second(QDir(dir).filePath(QStringLiteral("Title (2).pdf")));
    QVERIFY(second.open(QIODevice::WriteOnly));
    second.close();
    QCOMPARE(PdfExporter::downloadsPath(QStringLiteral("Title"), dir),
             QDir(dir).filePath(QStringLiteral("Title (3).pdf")));
}

QTEST_MAIN(PdfTest)
#include "tst_pdf.moc"
