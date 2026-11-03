#pragma once

#include <QFile>
#include <QString>

inline QString readText(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll());
}
