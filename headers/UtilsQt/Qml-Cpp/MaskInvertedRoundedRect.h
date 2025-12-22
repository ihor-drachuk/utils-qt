/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QQuickItem>

class MaskInvertedRoundedRect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
public:
    static void registerTypes();

    explicit MaskInvertedRoundedRect(QQuickItem* parent = nullptr);

    bool contains(const QPointF& point) const override;

    qreal radius() const;

public slots:
    void setRadius(qreal value);

signals:
    void radiusChanged(qreal radius);

private:
    qreal m_radius = 0;
};
