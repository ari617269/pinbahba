#include "core/PdfExporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>

#include "core/Markdown.h"
#include "core/ResourceText.h"
#include "core/Theme.h"

bool PdfExporter::exportTo(const QString& markdown, const QString& title, const QString& path) {
    QPdfWriter pdf(path);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    pdf.setResolution(300);
    pdf.setTitle(title);

    QTextDocument doc;
    doc.setDefaultStyleSheet(readText(QStringLiteral(":/print.css")));
    doc.setDefaultFont(Theme::serifFont(11.5));
    doc.setHtml(Markdown::toHtml(markdown));
    doc.setPageSize(pdf.pageLayout().paintRectPixels(pdf.resolution()).size());
    doc.print(&pdf);

    return QFileInfo(path).size() > 0;
}

QString PdfExporter::downloadsPath(const QString& title, const QString& dir) {
    QString base = title;
    base.replace(QLatin1Char('/'), QLatin1Char('-'));

    const QDir directory(dir);
    QString candidate = directory.filePath(base + QStringLiteral(".pdf"));
    for (int n = 2; QFileInfo::exists(candidate); ++n) {
        candidate = directory.filePath(base + QStringLiteral(" (%1).pdf").arg(n));
    }
    return candidate;
}
