/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QVector>
#include <QPolygonF>
#include <QPointF>
#include <QRectF>
#include <utils-cpp/copy_move.h>
#include <utils-cpp/pimpl.h>

class Polygon : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QPolygonF> polygons READ polygons WRITE setPolygons NOTIFY polygonsChanged)
    Q_PROPERTY(QPointF offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(bool negativeOffset READ negativeOffset WRITE setNegativeOffset NOTIFY negativeOffsetChanged)
    NO_COPY_MOVE(Polygon);
public:
    static void registerTypes();

    Polygon(QObject* parent = nullptr);
    ~Polygon() override;

    Q_INVOKABLE bool intersectsWithPoint(const QPointF& value) const;
    Q_INVOKABLE bool intersectsWithRect(const QRectF& value) const;

// --- Properties support ---
public:
    const QVector<QPolygonF>& polygons() const;
    QPointF offset() const;
    bool negativeOffset() const;

public slots:
    void setPolygons(const QVector<QPolygonF>& value);
    void setOffset(QPointF value);
    void setNegativeOffset(bool value);

signals:
    void polygonsChanged(const QVector<QPolygonF>& polygons);
    void offsetChanged(QPointF offset);
    void negativeOffsetChanged(bool negativeOffset);
// -------

private:
    void recalculate();

private:
    DECLARE_PIMPL
};
