#pragma once
#include <utils-qt/qml-cpp/Multibinding/Transformers/AbstractTransformer.h>

class ScaleNum : public AbstractTransformer
{
    Q_OBJECT
public:
    Q_PROPERTY(qreal factor READ factor WRITE setFactor NOTIFY factorChanged)
    Q_PROPERTY(bool roundOnRead READ roundOnRead WRITE setRoundOnRead NOTIFY roundOnReadChanged)
    Q_PROPERTY(bool roundOnWrite READ roundOnWrite WRITE setRoundOnWrite NOTIFY roundOnWriteChanged)

    static void registerTypes(const char* url);

    ScaleNum(QObject* parent = nullptr): AbstractTransformer(parent) { }
    QVariant readConverter(const QVariant& value) const override;
    QVariant writeConverter(const QVariant& newValue, const QVariant&) const override;

public:
    qreal factor() const { return m_factor; }
    bool roundOnRead() const { return m_roundOnRead; }
    bool roundOnWrite() const { return m_roundOnWrite; }

public slots:
    void setFactor(qreal value);
    void setRoundOnRead(bool roundOnRead);
    void setRoundOnWrite(bool roundOnWrite);

signals:
    void factorChanged(qreal factor);
    void roundOnReadChanged(bool roundOnRead);
    void roundOnWriteChanged(bool roundOnWrite);

private:
    qreal m_factor { 1.0 };
    bool m_roundOnRead { false };
    bool m_roundOnWrite { false };
};
