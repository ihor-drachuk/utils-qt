#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/ScaleNum.h>
#include <QQmlEngine>


void ScaleNum::registerTypes()
{
    AbstractTransformer::registerTypes();
    qmlRegisterType<ScaleNum>("UtilsQt", 1, 0, "ScaleNum");
}

QVariant ScaleNum::readConverter(const QVariant& value) const
{
    switch (value.type()) {
        case QVariant::Type::Invalid:
            return value;

        case QVariant::Type::Int:
        // codechecker_intentional [clang-diagnostic-switch]
        case (QVariant::Type)QMetaType::Type::Float:
        case QVariant::Type::Double:
            if (m_roundOnRead) {
                return qRound(value.toDouble() / m_factor);
            } else {
                return value.toDouble() / m_factor;
            }

        default:
            qFatal("Not supported QVariant::Type");
            return QVariant();
    }
}

QVariant ScaleNum::writeConverter(const QVariant& newValue, const QVariant&) const
{
    switch (newValue.type()) {
        case QVariant::Type::Invalid:
            return newValue;

        case QVariant::Type::Int:
        // codechecker_intentional [clang-diagnostic-switch]
        case (QVariant::Type)QMetaType::Type::Float:
        case QVariant::Type::Double:
            if (m_roundOnWrite) {
                return qRound(newValue.toDouble() * m_factor);
            } else {
                return newValue.toDouble() * m_factor;
            }

        default:
            qFatal("Not supported QVariant::Type");
            return QVariant();
    }
}

void ScaleNum::setFactor(qreal value)
{
    if (qFuzzyCompare(m_factor, value)) return;
    Q_ASSERT( !qFuzzyIsNull(value) );

    emit beforeTransformerUpdated();
    m_factor = value;
    emit afterTransformerUpdated();

    emit factorChanged(m_factor);
}

void ScaleNum::setRoundOnRead(bool roundOnRead)
{
    if (m_roundOnRead == roundOnRead)
        return;

    emit beforeTransformerUpdated();
    m_roundOnRead = roundOnRead;
    emit afterTransformerUpdated();

    emit roundOnReadChanged(m_roundOnRead);
}

void ScaleNum::setRoundOnWrite(bool roundOnWrite)
{
    if (m_roundOnWrite == roundOnWrite)
        return;

    emit beforeTransformerUpdated();
    m_roundOnWrite = roundOnWrite;
    emit afterTransformerUpdated();

    emit roundOnWriteChanged(m_roundOnWrite);
}
