#include "ui/PreviewView.h"

#include <QFrame>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTextDocument>

#include "core/Markdown.h"
#include "core/ResourceText.h"
#include "ui/Column.h"

namespace {

constexpr int kColumnCharacters = 68;
constexpr int kVerticalInset = 48;

}  // namespace

PreviewView::PreviewView(QWidget* parent)
    : QTextBrowser(parent), m_palette(Theme::light()) {
    setObjectName(QStringLiteral("preview"));
    setFrameShape(QFrame::NoFrame);
    setFont(Theme::serifFontPx(17));
    setOpenExternalLinks(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    updateInsets();
}

void PreviewView::showMarkdown(const QString& markdown) {
    m_markdown = markdown;
    render();
}

void PreviewView::applyPalette(const Theme::Palette& palette) {
    m_palette = palette;
    setStyleSheet(QStringLiteral("QTextBrowser { background-color: %1; color: %2; border: none; }")
                      .arg(palette.background.name(), palette.text.name()));
    render();
}

void PreviewView::resizeEvent(QResizeEvent* event) {
    QTextBrowser::resizeEvent(event);
    updateInsets();
}

void PreviewView::mouseDoubleClickEvent(QMouseEvent* event) {
    QTextBrowser::mouseDoubleClickEvent(event);
    emit editRequested();
}

void PreviewView::render() {
    const QString extraCss = QStringLiteral(
                                 "pre { background-color: %1; } a { color: %2; } "
                                 "td, th { border-color: %3; }")
                                 .arg(m_palette.codeBackground.name(), m_palette.accent.name(),
                                      m_palette.border.name());
    document()->setDefaultStyleSheet(readText(QStringLiteral(":/preview.css")) + extraCss);

    QString html = Markdown::toHtml(m_markdown);
    html.replace(QStringLiteral("<table>"),
                 QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"6\">"));
    html.replace(QStringLiteral("<pre>"),
                 QStringLiteral("<pre style=\"font-family: Menlo; font-size: 12pt;\">"));
    html.replace(QStringLiteral("<code"), QStringLiteral("<code style=\"font-family: Menlo;\""));
    setHtml(html);
}

void PreviewView::updateInsets() {
    const int inset = columnInset(width(), font(), kColumnCharacters);
    setViewportMargins(inset, kVerticalInset, inset, kVerticalInset);
}
