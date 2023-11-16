/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/JsTransformer.h>
#include <QQmlEngine>
#include <QDebug>

QJSEngine* JsTransformer::s_jsEngine { nullptr };

void JsTransformer::registerTypes()
{
    AbstractTransformer::registerTypes();
    qmlRegisterType<JsTransformer>("UtilsQt", 1, 0, "JsTransformer");
}

JsTransformer::JsTransformer(QObject* parent)
    : AbstractTransformer(parent)
{
    Q_ASSERT(s_jsEngine != nullptr);
}

QVariant JsTransformer::readConverter(const QVariant& value) const
{
    if (m_onReadConverter.isCallable()) {
        const auto jsValue = m_onReadConverter.call({s_jsEngine->toScriptValue(value)});
        if (jsValue.isError())
            qCritical() << "Error in JS function" << jsValue.toString();
        return s_jsEngine->fromScriptValue<QVariant>(jsValue);
    } else {
        return value;
    }
}

QVariant JsTransformer::writeConverter(const QVariant& newValue, const QVariant& orig) const
{
    if (m_onWriteConverter.isCallable()) {
        const auto jsValue = m_onWriteConverter.call({
                                                         s_jsEngine->toScriptValue(newValue),
                                                         s_jsEngine->toScriptValue(orig)
                                                     });
        if (jsValue.isError())
            qCritical() << "Error in JS function" << jsValue.toString();
        return s_jsEngine->fromScriptValue<QVariant>(jsValue);
    } else {
        return newValue;
    }
}

void JsTransformer::setOnReadConverter(QJSValue onReadConverter)
{
    m_onReadConverter = onReadConverter;
    emit onReadConverterChanged(m_onReadConverter);
}

void JsTransformer::setOnWriteConverter(QJSValue onWriteConverter)
{
    m_onWriteConverter = onWriteConverter;
    emit onWriteConverterChanged(m_onWriteConverter);
}
