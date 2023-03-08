#include <UtilsQt/qml.h>

#include <UtilsQt/Qml/QmlControls.h>
#include <UtilsQt/Qml-Cpp/FileWatcher.h>
#include <UtilsQt/Qml-Cpp/PathElider.h>
#include <UtilsQt/Qml-Cpp/ListModelItemProxy.h>
#include <UtilsQt/Qml-Cpp/ListModelTools.h>
#include <UtilsQt/Qml-Cpp/Multibinding/Multibinding.h>
#include <UtilsQt/Qml-Cpp/Multibinding/MultibindingItem.h>
#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/AbstractTransformer.h>
#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/JsTransformer.h>
#include <UtilsQt/Qml-Cpp/Multibinding/Transformers/ScaleNum.h>
#include <UtilsQt/Qml-Cpp/NumericalValidator.h>
#include <UtilsQt/Qml-Cpp/NumericValidatorInt.h>
#include <UtilsQt/Qml-Cpp/QmlUtils.h>
#include <UtilsQt/Qml-Cpp/Geometry/Geometry.h>
#include <UtilsQt/Qml-Cpp/Geometry/Polygon.h>
#include <UtilsQt/mergedlistmodel.h>

#include <QQmlEngine>
#include <QGuiApplication>

namespace UtilsQt {
namespace Qml {

void registerAll(QQmlEngine& engine)
{
    QmlControls::init(engine);
    FileWatcher::registerTypes();
    PathElider::registerTypes();
    ListModelItemProxy::registerTypes();
    ListModelTools::registerTypes();
    Multibinding::registerTypes();
    MultibindingItem::registerTypes();
    AbstractTransformer::registerTypes();
    JsTransformer::registerTypes();
    JsTransformer::setJsEngine(&engine);
    ScaleNum::registerTypes();
    NumericalValidator::registerTypes();
    NumericValidatorInt::registerTypes();
    QmlUtils::registerTypes();
    Geometry::registerTypes();
    Polygon::registerTypes();
    MergedListModel::registerTypes();
}

} // namespace Qml
} // namespace UtilsQt
