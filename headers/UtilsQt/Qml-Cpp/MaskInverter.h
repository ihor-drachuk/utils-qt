/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QQuickItem>

class MaskInverter : public QQuickItem
{
    Q_OBJECT
public:
    static void registerTypes();

    explicit MaskInverter(QQuickItem* parent = nullptr);

    bool contains(const QPointF& point) const override;
};
