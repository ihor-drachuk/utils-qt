#include <utils-qt/qml.h>

#include <utils-qt/qml/qml-controls.h>
#include <utils-qt/qml-cpp/ListModelItemProxy.h>
#include <utils-qt/qml-cpp/ListModelTools.h>
#include <utils-qt/qml-cpp/Multibinding/Multibinding.h>
#include <utils-qt/qml-cpp/Multibinding/MultibindingItem.h>
#include <utils-qt/qml-cpp/Multibinding/Transformers/AbstractTransformer.h>
#include <utils-qt/qml-cpp/Multibinding/Transformers/JsTransformer.h>
#include <utils-qt/qml-cpp/Multibinding/Transformers/ScaleNum.h>
#include <utils-qt/qml-cpp/NumericalValidator.h>
#include <utils-qt/qml-cpp/QmlUtils.h>
#include <utils-qt/mergedlistmodel.h>

#include <QQmlEngine>
#include <QGuiApplication>

namespace UtilsQt {
namespace Qml {

void registerAll(QQmlEngine& engine)
{
    QmlControls::init(engine);
    ListModelItemProxy::registerTypes();
    ListModelTools::registerTypes();
    Multibinding::registerTypes();
    MultibindingItem::registerTypes();
    AbstractTransformer::registerTypes();
    JsTransformer::registerTypes();
    JsTransformer::setJsEngine(&engine);
    ScaleNum::registerTypes();
    NumericalValidator::registerTypes();
    QmlUtils::registerTypes();
    MergedListModel::registerTypes();
}

} // namespace Qml
} // namespace UtilsQt
