#pragma once

#include <QStandardPaths>
#include <QString>

class PdfExporter {
public:
    static bool exportTo(const QString& markdown, const QString& title, const QString& path);
    static QString downloadsPath(
        const QString& title,
        const QString& dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
};
