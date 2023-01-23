#include <UtilsQt/Qml-Cpp/Geometry/Geometry.h>

#include <QQmlEngine>

struct Geometry::impl_t
{
};

Geometry& Geometry::instance()
{
    static Geometry object;
    return object;
}

void Geometry::registerTypes()
{
    qmlRegisterSingletonType<Geometry>("UtilsQt", 1, 0, "Geometry", [] (QQmlEngine *engine, QJSEngine *) -> QObject* {
        auto ret = &Geometry::instance();
        engine->setObjectOwnership(ret, QQmlEngine::CppOwnership);
        return ret;
    });
}

Geometry::Geometry()
{
    createImpl();
}

Geometry::~Geometry()
{
}

QPolygonF Geometry::polygonScale(const QPolygonF& polygon, qreal factor) const
{
    auto result = polygon;
    polygonScaleRef(result, factor);
    return result;
}

QVector<QPolygonF> Geometry::polygonsScale(const QVector<QPolygonF>& polygons, qreal factor) const
{
    auto result = polygons;
    polygonsScaleRef(result, factor);
    return result;
}

void Geometry::polygonScaleRef(QPolygonF& polygon, qreal factor) const
{
    for (auto& x : polygon)
        x *= factor;
}

void Geometry::polygonsScaleRef(QVector<QPolygonF>& polygons, qreal factor) const
{
    for (auto& polygon : polygons)
        for (auto& x : polygon)
            x *= factor;
}

bool Geometry::isPolygonRectangular(const QPolygonF& polygon) const
{
    double area = 0.0;
    int j = polygon.size() - 1;
    for (int i = 0; i < polygon.size(); i++) {
        area += (polygon[j].x() + polygon[i].x()) * (polygon[j].y() - polygon[i].y());
        j = i;
    }
    auto polygonArea = abs(area / 2.0);
    auto boundingRectArea = polygon.boundingRect().width() * polygon.boundingRect().height();
    return qFuzzyCompare(polygonArea, boundingRectArea);
}
