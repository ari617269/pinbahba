#include "core/Title.h"

#include <QStringList>

namespace Title {

QString fromText(const QString& text) {
    constexpr int kMaxLength = 60;

    for (const QString& raw : text.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        if (!line.startsWith(QLatin1Char('#'))) continue;

        int hashes = 0;
        while (hashes < line.size() && line.at(hashes) == QLatin1Char('#')) ++hashes;

        QString title = line.mid(hashes);
        for (const char marker : {'*', '_', '`'}) title.remove(QLatin1Char(marker));
        title = title.trimmed().left(kMaxLength).trimmed();
        return title.isEmpty() ? QStringLiteral("untitled") : title;
    }

    return QStringLiteral("untitled");
}

}  // namespace Title
