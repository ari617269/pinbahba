#pragma once

#include <QWidget>

#include "core/Theme.h"

class QEnterEvent;
class QGraphicsOpacityEffect;
class QLabel;
class QPropertyAnimation;
class QTimer;
class QToolButton;

class OverlayBar : public QWidget {
    Q_OBJECT

public:
    explicit OverlayBar(QWidget* parent = nullptr);

    void applyPalette(const Theme::Palette& palette);
    void showSaved();

signals:
    void editRequested();
    void exportRequested();
    void revealRequested();
    void contentChanged();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void fadeTo(qreal opacity);
    void setSavedState(bool saved);

    QToolButton* m_edit = nullptr;
    QLabel* m_separator = nullptr;
    QToolButton* m_pdf = nullptr;
    QToolButton* m_saved = nullptr;
    QGraphicsOpacityEffect* m_effect = nullptr;
    QPropertyAnimation* m_animation = nullptr;
    QTimer* m_savedTimer = nullptr;
};
