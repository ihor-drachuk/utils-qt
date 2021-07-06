#include <utils-qt/qml-cpp/Multibinding/Transformers/AbstractTransformer.h>
#include <QQmlEngine>

void AbstractTransformer::registerTypes(const char* url)
{
    qmlRegisterUncreatableType<AbstractTransformer>(url, 1, 0, "AbstractTransformer", "Uncreatable type");
}
