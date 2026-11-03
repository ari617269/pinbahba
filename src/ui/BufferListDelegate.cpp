#include "ui/BufferListDelegate.h"

#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>

#include "core/BufferStore.h"
#include "core/RelativeTime.h"

namespace {

constexpr int kRowHeight = 56;
constexpr int kHorizontalPadding = 14;
constexpr int kVerticalPadding = 8;
constexpr int kTitleHeight = 20;
constexpr int kCloseWidth = 12;

}  // namespace

BufferListDelegate::BufferListDelegate(QObject* parent)
    : QStyledItemDelegate(parent), m_palette(Theme::light()) {}

void BufferListDelegate::applyPalette(const Theme::Palette& palette) {
    m_palette = palette;
}

void BufferListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
    painter->save();

    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    if (selected) painter->fillRect(option.rect, m_palette.codeBackground);

    const QRect content = option.rect.adjusted(kHorizontalPadding, kVerticalPadding,
                                               -kHorizontalPadding, -kVerticalPadding);
    const QString title = index.data(BufferStore::TitleRole).toString();
    const QDateTime updated = index.data(BufferStore::UpdatedRole).toDateTime();

    const QFont titleFont = Theme::uiFontPx(13);
    painter->setFont(titleFont);
    painter->setPen(m_palette.text);
    const QRect titleRect(content.left(), content.top(),
                          content.width() - (hovered ? kCloseWidth : 0), kTitleHeight);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

    painter->setFont(Theme::uiFontPx(11));
    painter->setPen(m_palette.muted);
    const QRect timeRect(content.left(), content.top() + kTitleHeight + 2, content.width(), 16);
    painter->drawText(timeRect, Qt::AlignLeft | Qt::AlignVCenter,
                      RelativeTime::describe(updated, QDateTime::currentDateTime()));

    if (hovered) {
        const QRect closeRect(content.right() - kCloseWidth + 1, content.top(), kCloseWidth,
                              kTitleHeight);
        painter->drawText(closeRect, Qt::AlignCenter, QString::fromUtf8("×"));
    }

    painter->restore();
}

QSize BufferListDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(0, kRowHeight);
}
