#include "ui/EditorView.h"

#include <QFrame>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QTextBlockFormat>
#include <QTextCursor>

#include "ui/Column.h"

namespace {

constexpr int kColumnCharacters = 72;
constexpr int kVerticalInset = 48;
constexpr qreal kLineHeightPercent = 175;

}  // namespace

EditorView::EditorView(QWidget* parent) : QTextEdit(parent) {
    setObjectName(QStringLiteral("editor"));
    setAcceptRichText(false);
    setFrameShape(QFrame::NoFrame);
    setFont(Theme::monoFontPx(15));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    updateInsets();
}

void EditorView::setText(const QString& text) {
    setPlainText(text);

    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);
    QTextBlockFormat format;
    format.setLineHeight(kLineHeightPercent, QTextBlockFormat::ProportionalHeight);
    cursor.mergeBlockFormat(format);
}

void EditorView::applyPalette(const Theme::Palette& palette) {
    setStyleSheet(QStringLiteral("QTextEdit { background-color: %1; color: %2; border: none; }")
                      .arg(palette.background.name(), palette.text.name()));
}

void EditorView::resizeEvent(QResizeEvent* event) {
    QTextEdit::resizeEvent(event);
    updateInsets();
}

void EditorView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Tab && event->modifiers() == Qt::NoModifier) {
        insertPlainText(QStringLiteral("  "));
        return;
    }
    QTextEdit::keyPressEvent(event);
}

void EditorView::updateInsets() {
    const int inset = columnInset(width(), font(), kColumnCharacters);
    setViewportMargins(inset, kVerticalInset, inset, kVerticalInset);
}
