/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QPolygonF>
#include <QVector>
#include <utils-cpp/default_ctor_ops.h>
#include <utils-cpp/pimpl.h>


// codechecker_intentional [cppcoreguidelines-virtual-class-destructor]
// This is a singleton instance and should not be instanciated or deleted
class Geometry : public QObject
{
    Q_OBJECT
    NO_COPY_MOVE(Geometry);
public:
    static Geometry& instance();
    static void registerTypes();

    Q_INVOKABLE [[nodiscard]] QPolygonF polygonScale(const QPolygonF& polygon, qreal xFactor, qreal yFactor) const;
    Q_INVOKABLE [[nodiscard]] QVector<QPolygonF> polygonsScale(const QVector<QPolygonF>& polygons, qreal xFactor, qreal yFactor) const;
    Q_INVOKABLE void polygonScaleRef(QPolygonF& polygon, qreal xFactor, qreal yFactor) const;
    Q_INVOKABLE void polygonsScaleRef(QVector<QPolygonF>& polygons, qreal xFactor, qreal yFactor) const;
    Q_INVOKABLE bool isPolygonRectangular(const QPolygonF& polygon) const;

// --- Properties support ---
public:

public slots:

signals:
// -------

private:
    Geometry();
    ~Geometry() override;

private:
    DECLARE_PIMPL
};
