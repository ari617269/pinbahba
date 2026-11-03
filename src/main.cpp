#include <optional>

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>

#include "core/BufferStore.h"
#include "platform/SingleInstance.h"
#include "ui/MainWindow.h"

namespace {

QSize parseSize(const QString& text, const QSize& fallback) {
    const QStringList parts = text.split(QLatin1Char('x'));
    if (parts.size() != 2) return fallback;
    bool okW = false, okH = false;
    const int w = parts.at(0).toInt(&okW);
    const int h = parts.at(1).toInt(&okH);
    return (okW && okH) ? QSize(w, h) : fallback;
}

MainWindow::Mode parseMode(const QString& text) {
    return text == QLatin1String("edit") ? MainWindow::Mode::Edit : MainWindow::Mode::View;
}

MainWindow::ThemeChoice parseTheme(const QString& text) {
    if (text == QLatin1String("light")) return MainWindow::ThemeChoice::Light;
    if (text == QLatin1String("dark")) return MainWindow::ThemeChoice::Dark;
    return MainWindow::ThemeChoice::System;
}

QString instanceName() {
    return QStringLiteral("md-%1").arg(qEnvironmentVariable("USER", QStringLiteral("user")));
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--screenshot") == 0) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
            break;
        }
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("md"));
    QApplication::setOrganizationName(QStringLiteral("md"));
    QGuiApplication::setDesktopFileName(QStringLiteral("md"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Ultraminimal markdown buffers"));
    parser.addHelpOption();
    const QCommandLineOption dataDirOpt(QStringLiteral("data-dir"),
                                        QStringLiteral("Storage directory for buffers."),
                                        QStringLiteral("path"));
    const QCommandLineOption downloadsDirOpt(QStringLiteral("downloads-dir"),
                                             QStringLiteral("Directory for exported PDFs."),
                                             QStringLiteral("path"));
    const QCommandLineOption settingsOpt(QStringLiteral("settings-file"),
                                         QStringLiteral("Settings file (INI) to use."),
                                         QStringLiteral("path"));
    const QCommandLineOption screenshotOpt(QStringLiteral("screenshot"),
                                           QStringLiteral("Save a PNG of the window and exit."),
                                           QStringLiteral("path"));
    const QCommandLineOption modeOpt(QStringLiteral("mode"),
                                     QStringLiteral("Initial mode: edit or view."),
                                     QStringLiteral("mode"));
    const QCommandLineOption themeOpt(QStringLiteral("theme"),
                                      QStringLiteral("Colour theme: system, light or dark."),
                                      QStringLiteral("theme"), QStringLiteral("system"));
    const QCommandLineOption sizeOpt(QStringLiteral("size"),
                                     QStringLiteral("Window size as WIDTHxHEIGHT."),
                                     QStringLiteral("size"));
    parser.addOption(dataDirOpt);
    parser.addOption(downloadsDirOpt);
    parser.addOption(settingsOpt);
    parser.addOption(screenshotOpt);
    parser.addOption(modeOpt);
    parser.addOption(themeOpt);
    parser.addOption(sizeOpt);
    parser.process(app);

    const bool screenshot = parser.isSet(screenshotOpt);
    const QString dataDir = parser.isSet(dataDirOpt)
                                ? parser.value(dataDirOpt)
                                : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString downloadsDir =
        parser.isSet(downloadsDirOpt)
            ? parser.value(downloadsDirOpt)
            : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QSettings settings = parser.isSet(settingsOpt)
                             ? QSettings(parser.value(settingsOpt), QSettings::IniFormat)
                             : QSettings();

    SingleInstance instance;
    if (!screenshot && !instance.claim(instanceName())) return 0;

    BufferStore store(dataDir);
    MainWindow window(&store,
                      parser.isSet(modeOpt) ? std::optional(parseMode(parser.value(modeOpt)))
                                            : std::nullopt,
                      parseTheme(parser.value(themeOpt)), downloadsDir, &settings);
    app.setQuitOnLastWindowClosed(!window.enableTray());
    if (parser.isSet(sizeOpt)) {
        window.resize(parseSize(parser.value(sizeOpt), QSize(1100, 700)));
    }

    QObject::connect(&app, &QApplication::aboutToQuit, &store, &BufferStore::flush);
    QObject::connect(&app, &QApplication::aboutToQuit, &window, &MainWindow::saveState);

    if (screenshot) {
        window.show();
        QCoreApplication::processEvents();
        const bool saved = window.grab().save(parser.value(screenshotOpt));
        return saved ? 0 : 1;
    }

    QObject::connect(&instance, &SingleInstance::showRequested, &window, &MainWindow::showWindow);
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &window,
                     [&window](Qt::ApplicationState state) {
                         if (state == Qt::ApplicationActive && !window.isVisible()) {
                             window.showWindow();
                         }
                     });

    window.show();
    return app.exec();
}
