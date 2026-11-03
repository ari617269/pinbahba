#pragma once

#include <QDateTime>
#include <QString>

namespace RelativeTime {

QString describe(const QDateTime& then, const QDateTime& now);

}
