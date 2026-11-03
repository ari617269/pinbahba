#include <QFile>
#include <QStringList>
#include <QTest>

namespace {

QString readSource(const QString& relative) {
    QFile file(QStringLiteral(MD_SOURCE_DIR) + QLatin1Char('/') + relative);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(file.readAll());
}

}  // namespace

class PackagingTest : public QObject {
    Q_OBJECT

private slots:
    void macInfoPlistDeclaresBundle();
    void linuxDesktopEntryLaunchesMd();
};

void PackagingTest::macInfoPlistDeclaresBundle() {
    const QString plist = readSource(QStringLiteral("packaging/macos/Info.plist.in"));
    QVERIFY2(!plist.isEmpty(), "packaging/macos/Info.plist.in is missing");

    const QStringList keys = {
        QStringLiteral("CFBundleExecutable"),
        QStringLiteral("CFBundleIdentifier"),
        QStringLiteral("CFBundleName"),
        QStringLiteral("CFBundlePackageType"),
        QStringLiteral("CFBundleShortVersionString"),
        QStringLiteral("CFBundleVersion"),
        QStringLiteral("LSApplicationCategoryType"),
        QStringLiteral("LSMinimumSystemVersion"),
        QStringLiteral("NSHighResolutionCapable"),
        QStringLiteral("NSPrincipalClass"),
    };
    for (const QString& key : keys) {
        QVERIFY2(plist.contains(QStringLiteral("<key>%1</key>").arg(key)),
                 qPrintable(QStringLiteral("missing key %1").arg(key)));
    }

    QVERIFY(plist.contains(QStringLiteral("<string>${MACOSX_BUNDLE_GUI_IDENTIFIER}</string>")));
    QVERIFY(plist.contains(QStringLiteral("<string>APPL</string>")));
    QVERIFY(plist.contains(QStringLiteral("<string>12.0</string>")));
    QVERIFY(plist.contains(QStringLiteral("<true/>")));
}

void PackagingTest::linuxDesktopEntryLaunchesMd() {
    const QString desktop = readSource(QStringLiteral("packaging/linux/md.desktop"));
    QVERIFY2(!desktop.isEmpty(), "packaging/linux/md.desktop is missing");

    const QStringList lines = desktop.split(QLatin1Char('\n'));
    QVERIFY(lines.contains(QStringLiteral("[Desktop Entry]")));
    QVERIFY(lines.contains(QStringLiteral("Type=Application")));
    QVERIFY(lines.contains(QStringLiteral("Name=md")));
    QVERIFY(lines.contains(QStringLiteral("Exec=md")));
    QVERIFY(lines.contains(QStringLiteral("Icon=md")));
    QVERIFY(lines.contains(QStringLiteral("StartupWMClass=md")));
    QVERIFY(lines.contains(QStringLiteral("Terminal=false")));
}

QTEST_APPLESS_MAIN(PackagingTest)
#include "tst_packaging.moc"
