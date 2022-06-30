#pragma once
#include <QObject>
#include <utils-cpp/pimpl.h>

class Geometry : public QObject
{
    Q_OBJECT
public:
    static Geometry& instance();
    static void registerTypes();

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
