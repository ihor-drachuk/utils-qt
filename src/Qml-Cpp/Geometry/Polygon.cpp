/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/Geometry/Polygon.h>

#include <QQmlEngine>
#include <QVector>
#include <QPolygonF>
#include <QVariantList>

struct Polygon::impl_t
{
    QPolygonF combinedPolygon;
    QVector<QPolygonF> translatedPolygons;

    QVector<QPolygonF> polygons;
    QPointF offset;
    bool negativeOffset { false };
};

void Polygon::registerTypes()
{
    qmlRegisterType<Polygon>("UtilsQt", 1, 0, "Polygon");
}

Polygon::Polygon(QObject* parent)
    : QObject(parent)
{
    createImpl();
}

Polygon::~Polygon()
{
}

bool Polygon::intersectsWithPoint(const QPointF& value) const
{
    return impl().combinedPolygon.containsPoint(value, Qt::FillRule::OddEvenFill);
}

bool Polygon::intersectsWithRect(const QRectF& value) const
{
    const QPolygonF rectPolygon(value);

    for (const auto& x : std::as_const(impl().translatedPolygons))
        if (rectPolygon.intersects(x))
            return true;

    return false;
}

const QVector<QPolygonF>& Polygon::polygons() const
{
    return impl().polygons;
}

void Polygon::setPolygons(const QVector<QPolygonF>& value)
{
    if (impl().polygons == value)
        return;
    impl().polygons = value;
    recalculate();
    emit polygonsChanged(impl().polygons);
}

QPointF Polygon::offset() const
{
    return impl().offset;
}

void Polygon::setOffset(QPointF value)
{
    if (impl().offset == value)
        return;
    impl().offset = value;
    recalculate();
    emit offsetChanged(impl().offset);
}

bool Polygon::negativeOffset() const
{
    return impl().negativeOffset;
}

void Polygon::setNegativeOffset(bool value)
{
    if (impl().negativeOffset == value)
        return;
    impl().negativeOffset = value;
    recalculate();
    emit negativeOffsetChanged(impl().negativeOffset);
}

void Polygon::recalculate()
{
    impl().combinedPolygon.clear();
    impl().translatedPolygons = impl().polygons;

    const QPointF offset (impl().offset * (impl().negativeOffset ? -1 : 1));

    for (const auto& x : std::as_const(impl().polygons))
        impl().combinedPolygon = impl().combinedPolygon.united(x);

    impl().combinedPolygon.translate(offset);

    for (auto& x : impl().translatedPolygons)
        x.translate(offset);
}
