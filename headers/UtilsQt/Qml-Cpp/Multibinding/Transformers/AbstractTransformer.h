#pragma once
#include <QObject>
#include <QVariant>

class AbstractTransformer : public QObject
{
    Q_OBJECT
public:
    static void registerTypes();

    AbstractTransformer(QObject* parent): QObject(parent) { }
    virtual QVariant readConverter(const QVariant& value) const = 0;
    virtual QVariant writeConverter(const QVariant& newValue, const QVariant& orig) const = 0;

signals:
    void beforeTransformerUpdated();
    void afterTransformerUpdated();
};
