/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/MaskInvertedRoundedRect.h>

#include <QQmlEngine>

void MaskInvertedRoundedRect::registerTypes()
{
    qmlRegisterType<MaskInvertedRoundedRect>("UtilsQt", 1, 0, "MaskInvertedRoundedRect");
}

MaskInvertedRoundedRect::MaskInvertedRoundedRect(QQuickItem* parent)
    : QQuickItem(parent)
{
}

bool MaskInvertedRoundedRect::contains(const QPointF& point) const
{
    // Returns true if point is in a rounded corner (OUTSIDE the rounded rect)
    const qreal r = m_radius;
    if (r <= 0)
        return false;  // No corners to block

    const qreal w = width();
    const qreal h = height();
    const qreal x = point.x();
    const qreal y = point.y();

    // Fast path: not in any corner region
    if (x >= r && x <= w - r)
        return false;
    if (y >= r && y <= h - r)
        return false;

    // Slow path: check if point is outside the corner circle
    qreal dx, dy;
    if (x < r) {
        dx = x - r;
        dy = y < r ? y - r :       // top-left
                     y - h + r;    // bottom-left
    } else {
        dx = x - w + r;
        dy = y < r ? y - r :       // top-right
                     y - h + r;    // bottom-right
    }

    return dx * dx + dy * dy > r * r;  // Outside circle = in corner
}

qreal MaskInvertedRoundedRect::radius() const
{
    return m_radius;
}

void MaskInvertedRoundedRect::setRadius(qreal value)
{
    if (qFuzzyCompare(m_radius, value))
        return;

    m_radius = value;
    emit radiusChanged(m_radius);
}
