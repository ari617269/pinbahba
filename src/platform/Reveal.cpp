#include "platform/Reveal.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

namespace Platform {

void revealInFileManager(const QString& path) {
#ifdef Q_OS_MACOS
    QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-R"), path});
#else
    const QFileInfo info(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? path : info.absolutePath()));
#endif
}

}  // namespace Platform
