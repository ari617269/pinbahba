#pragma once

#include <algorithm>

#include <QFont>
#include <QFontMetricsF>
#include <QString>

inline int columnInset(int widgetWidth, const QFont& font, int characters) {
    const int columnWidth =
        qRound(QFontMetricsF(font).horizontalAdvance(QStringLiteral("0")) * characters);
    return std::max(0, (widgetWidth - columnWidth) / 2);
}
