#pragma once

#include <QTextBrowser>

#include "core/Theme.h"

class QMouseEvent;

class PreviewView : public QTextBrowser {
    Q_OBJECT

public:
    explicit PreviewView(QWidget* parent = nullptr);

    void showMarkdown(const QString& markdown);
    void applyPalette(const Theme::Palette& palette);

signals:
    void editRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void render();
    void updateInsets();

    QString m_markdown;
    Theme::Palette m_palette;
};
