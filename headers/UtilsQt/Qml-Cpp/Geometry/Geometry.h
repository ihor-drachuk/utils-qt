#pragma once
#include <QObject>
#include <QPolygonF>
#include <QVector>
#include <utils-cpp/copy_move.h>
#include <utils-cpp/pimpl.h>

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
