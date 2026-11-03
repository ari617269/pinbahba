#pragma once

#include <QColor>
#include <QFont>
#include <QStringList>

namespace Theme {

struct Palette {
    QColor background;
    QColor text;
    QColor muted;
    QColor codeBackground;
    QColor border;
    QColor accent;
};

inline Palette light() {
    return {QColor("#fbfaf7"), QColor("#1f1f1f"), QColor("#8a8a8a"), QColor("#f1efe9"),
            QColor("#d6d3cb"), QColor("#2f5fb3")};
}

inline Palette dark() {
    return {QColor("#1c1c1e"), QColor("#e6e6e6"), QColor("#7d7d80"), QColor("#2a2a2d"),
            QColor("#3a3a3d"), QColor("#8ab4ff")};
}

inline QStringList serifStack() {
    return {QStringLiteral("Charter"), QStringLiteral("Iowan Old Style"),
            QStringLiteral("Georgia"), QStringLiteral("serif")};
}

inline QStringList monoStack() {
    return {QStringLiteral("SF Mono"), QStringLiteral("JetBrains Mono"), QStringLiteral("Menlo")};
}

inline QStringList uiStack() {
    return {QStringLiteral("-apple-system"), QStringLiteral("Helvetica Neue"),
            QStringLiteral("sans-serif")};
}

inline QFont fontFor(const QStringList& families, double pointSize) {
    QFont font;
    font.setFamilies(families);
    font.setPointSizeF(pointSize);
    return font;
}

inline QFont fontForPixels(const QStringList& families, int pixelSize) {
    QFont font;
    font.setFamilies(families);
    font.setPixelSize(pixelSize);
    return font;
}

inline QFont serifFont(double pointSize) { return fontFor(serifStack(), pointSize); }
inline QFont monoFont(double pointSize) {
    QFont font = fontFor(monoStack(), pointSize);
    font.setStyleHint(QFont::Monospace);
    return font;
}
inline QFont uiFont(double pointSize) { return fontFor(uiStack(), pointSize); }

inline QFont serifFontPx(int pixelSize) { return fontForPixels(serifStack(), pixelSize); }
inline QFont monoFontPx(int pixelSize) {
    QFont font = fontForPixels(monoStack(), pixelSize);
    font.setStyleHint(QFont::Monospace);
    return font;
}
inline QFont uiFontPx(int pixelSize) { return fontForPixels(uiStack(), pixelSize); }

}  // namespace Theme
