#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/AbstractTransformer.h>
#include <QQmlEngine>

void AbstractTransformer::registerTypes()
{
    qmlRegisterUncreatableType<AbstractTransformer>("UtilsQt", 1, 0, "AbstractTransformer", "Uncreatable type");
}
