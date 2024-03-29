/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/ScaleNum.h>
#include <QQmlEngine>
#include <UtilsQt/qvariant_traits.h>

void ScaleNum::registerTypes()
{
    AbstractTransformer::registerTypes();
    qmlRegisterType<ScaleNum>("UtilsQt", 1, 0, "ScaleNum");
}

QVariant ScaleNum::readConverter(const QVariant& value) const
{
    if (UtilsQt::QVariantTraits::isUnknown(value))
        return value;

    if (UtilsQt::QVariantTraits::isInteger(value) || UtilsQt::QVariantTraits::isFloat(value)) {
        const auto result = value.toDouble() / m_factor;
        return m_roundOnRead ? qRound(result) : result;
    }

    qFatal("Not supported QVariant::Type");
    return QVariant();
}

QVariant ScaleNum::writeConverter(const QVariant& newValue, const QVariant&) const
{
    if (UtilsQt::QVariantTraits::isUnknown(newValue))
        return newValue;

    if (UtilsQt::QVariantTraits::isInteger(newValue) || UtilsQt::QVariantTraits::isFloat(newValue)) {
        const auto result = newValue.toDouble() * m_factor;
        return m_roundOnWrite ? qRound(result) : result;
    }

    qFatal("Not supported QVariant::Type");
    return QVariant();
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
