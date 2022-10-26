#pragma once

#include <QObject>
#include <utils-cpp/pimpl.h>

#include <QMetaObject>

class ExclusiveGroupIndex : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* current READ current WRITE setCurrent NOTIFY currentChanged)

public:
    explicit ExclusiveGroupIndex(QObject* parent = 0);

    static void registerTypes();

    QObject* current() const;
    void setCurrent(QObject* obj);

public Q_SLOTS:
    void dispatchContainer(QObject* obj);
    void clearContainer(QObject* obj);

Q_SIGNALS:
    void currentChanged();

private Q_SLOTS:
    void updateCurrent();

private:
    DECLARE_PIMPL
};
