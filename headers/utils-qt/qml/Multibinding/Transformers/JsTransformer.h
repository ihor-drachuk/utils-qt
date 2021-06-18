#pragma once
#include <utils-qt/qml/Multibinding/Transformers/AbstractTransformer.h>
#include <QJSValue>

class QJSEngine;

class JsTransformer : public AbstractTransformer
{
    Q_OBJECT
public:
    Q_PROPERTY(QJSValue onReadConverter READ onReadConverter WRITE setOnReadConverter NOTIFY onReadConverterChanged)
    Q_PROPERTY(QJSValue onWriteConverter READ onWriteConverter WRITE setOnWriteConverter NOTIFY onWriteConverterChanged)

    static void registerTypes(const char* url);
    static void setJsEngine(QJSEngine* jsEngine) { s_jsEngine = jsEngine; }

    JsTransformer(QObject* parent = nullptr);
    QVariant readConverter(const QVariant& value) const override;
    QVariant writeConverter(const QVariant& newValue, const QVariant& orig) const override;

public:
    QJSValue onReadConverter() const { return m_onReadConverter; }
    QJSValue onWriteConverter() const { return m_onWriteConverter; }

public slots:
    void setOnReadConverter(QJSValue onReadConverter);
    void setOnWriteConverter(QJSValue onWriteConverter);

signals:
    void onReadConverterChanged(QJSValue onReadConverter);
    void onWriteConverterChanged(QJSValue onWriteConverter);

private:
    static QJSEngine* s_jsEngine;
    mutable QJSValue m_onReadConverter;
    mutable QJSValue m_onWriteConverter;
};
