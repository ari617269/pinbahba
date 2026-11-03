#include "ui/OverlayBar.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>

namespace {

constexpr qreal kRestingOpacity = 0.25;
constexpr qreal kHoverOpacity = 1.0;
constexpr int kFadeMs = 180;
constexpr int kSavedMs = 3000;

}  // namespace

OverlayBar::OverlayBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("overlay"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_edit = new QToolButton(this);
    m_edit->setObjectName(QStringLiteral("editButton"));
    m_edit->setText(QStringLiteral("edit"));
    m_edit->setAutoRaise(true);

    m_separator = new QLabel(QStringLiteral("/"), this);

    m_pdf = new QToolButton(this);
    m_pdf->setObjectName(QStringLiteral("pdfButton"));
    m_pdf->setText(QStringLiteral("pdf ↓"));
    m_pdf->setAutoRaise(true);

    m_saved = new QToolButton(this);
    m_saved->setObjectName(QStringLiteral("savedButton"));
    m_saved->setText(QStringLiteral("saved · reveal"));
    m_saved->setAutoRaise(true);
    m_saved->hide();

    layout->addWidget(m_edit);
    layout->addWidget(m_separator);
    layout->addWidget(m_pdf);
    layout->addWidget(m_saved);

    connect(m_edit, &QToolButton::clicked, this, &OverlayBar::editRequested);
    connect(m_pdf, &QToolButton::clicked, this, &OverlayBar::exportRequested);
    connect(m_saved, &QToolButton::clicked, this, &OverlayBar::revealRequested);

    m_savedTimer = new QTimer(this);
    m_savedTimer->setSingleShot(true);
    m_savedTimer->setInterval(kSavedMs);
    connect(m_savedTimer, &QTimer::timeout, this, [this] { setSavedState(false); });

    m_effect = new QGraphicsOpacityEffect(this);
    m_effect->setOpacity(kRestingOpacity);
    setGraphicsEffect(m_effect);

    m_animation = new QPropertyAnimation(m_effect, "opacity", this);
    m_animation->setDuration(kFadeMs);
}

void OverlayBar::applyPalette(const Theme::Palette& palette) {
    setStyleSheet(QStringLiteral("QToolButton { color: %1; border: none; padding: 2px 4px; }"
                                 "QLabel { color: %1; }")
                      .arg(palette.muted.name()));
}

void OverlayBar::showSaved() {
    setSavedState(true);
    m_savedTimer->start();
}

void OverlayBar::enterEvent(QEnterEvent* event) {
    QWidget::enterEvent(event);
    fadeTo(kHoverOpacity);
}

void OverlayBar::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    fadeTo(kRestingOpacity);
}

void OverlayBar::fadeTo(qreal opacity) {
    m_animation->stop();
    m_animation->setStartValue(m_effect->opacity());
    m_animation->setEndValue(opacity);
    m_animation->start();
}

void OverlayBar::setSavedState(bool saved) {
    m_edit->setVisible(!saved);
    m_separator->setVisible(!saved);
    m_pdf->setVisible(!saved);
    m_saved->setVisible(saved);
    adjustSize();
    emit contentChanged();
}
