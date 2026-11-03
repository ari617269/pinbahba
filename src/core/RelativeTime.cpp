#include "core/RelativeTime.h"

#include <QLocale>

namespace RelativeTime {

QString describe(const QDateTime& then, const QDateTime& now) {
    constexpr qint64 kMinute = 60;
    constexpr qint64 kHour = 60 * kMinute;
    constexpr qint64 kDay = 24 * kHour;
    constexpr qint64 kWeek = 7 * kDay;

    const qint64 seconds = then.secsTo(now);
    if (seconds < kMinute) return QStringLiteral("just now");
    if (seconds < kHour) return QStringLiteral("%1m").arg(seconds / kMinute);
    if (seconds < kDay) return QStringLiteral("%1h").arg(seconds / kHour);
    if (seconds < kWeek) return QStringLiteral("%1d").arg(seconds / kDay);
    return QLocale::c().toString(then.date(), QStringLiteral("MMM d"));
}

}  // namespace RelativeTime
