/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/qml.h>

#include <UtilsQt/Qml/QmlControls.h>
#include <UtilsQt/Qml-Cpp/SteadyTimer.h>
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
#include <UtilsQt/Qml-Cpp/FilterBehavior.h>
#include <UtilsQt/Qml-Cpp/MaskInverter.h>
#include <UtilsQt/Qml-Cpp/MaskInvertedRoundedRect.h>
#include <UtilsQt/Qml-Cpp/PropertyInterceptor.h>
#include <UtilsQt/MergedListModel.h>
#include <UtilsQt/PlusOneProxyModel.h>

#include <QQmlEngine>
#include <QGuiApplication>

namespace UtilsQt {
namespace Qml {

void registerAll(QQmlEngine& engine)
{
    QmlControls::init(engine);
    SteadyTimer::registerTypes();
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
    FilterBehavior::registerTypes();
    PropertyInterceptor::registerTypes();
    MergedListModel::registerTypes();
    PlusOneProxyModel::registerTypes();
    MaskInverter::registerTypes();
    MaskInvertedRoundedRect::registerTypes();
}

} // namespace Qml
} // namespace UtilsQt
