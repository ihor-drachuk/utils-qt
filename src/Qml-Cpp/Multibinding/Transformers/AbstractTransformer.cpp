/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/AbstractTransformer.h>
#include <QQmlEngine>

void AbstractTransformer::registerTypes()
{
    qmlRegisterUncreatableType<AbstractTransformer>("UtilsQt", 1, 0, "AbstractTransformer", "Uncreatable type");
}
