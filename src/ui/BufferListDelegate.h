#pragma once

#include <QStyledItemDelegate>

#include "core/Theme.h"

class BufferListDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit BufferListDelegate(QObject* parent = nullptr);

    void applyPalette(const Theme::Palette& palette);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    Theme::Palette m_palette;
};
