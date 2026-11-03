#pragma once

#include <QTextEdit>

#include "core/Theme.h"

class QKeyEvent;

class EditorView : public QTextEdit {
    Q_OBJECT

public:
    explicit EditorView(QWidget* parent = nullptr);

    void setText(const QString& text);
    void applyPalette(const Theme::Palette& palette);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateInsets();
};
