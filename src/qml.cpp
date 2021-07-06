#include "utils-qt/qml.h"

#include "utils-qt/qml/qml-controls.h"
#include "utils-qt/qml-cpp/ListModelItemProxy.h"
#include "utils-qt/qml-cpp/ListModelTools.h"
#include "utils-qt/qml-cpp/Multibinding/Multibinding.h"
#include "utils-qt/qml-cpp/Multibinding/MultibindingItem.h"
#include "utils-qt/qml-cpp/Multibinding/Transformers/AbstractTransformer.h"
#include "utils-qt/qml-cpp/Multibinding/Transformers/JsTransformer.h"
#include "utils-qt/qml-cpp/Multibinding/Transformers/ScaleNum.h"
#include "utils-qt/qml-cpp/NumericalValidator.h"
#include "utils-qt/qml-cpp/QmlUtils.h"

#include <QQmlEngine>

namespace UtilsQt {
namespace Qml {

void registerAll(QQmlEngine& engine)
{
    const char* const url = "UtilsQt";

    QmlControls::init(engine);
    ListModelItemProxy::registerTypes(url);
    ListModelTools::registerTypes(url);
    Multibinding::registerTypes(url);
    MultibindingItem::registerTypes(url);
    AbstractTransformer::registerTypes(url);
    JsTransformer::registerTypes(url);
    ScaleNum::registerTypes(url);
    NumericalValidator::registerTypes(url);
    QmlUtils::registerTypes(url);
}

} // namespace Qml
} // namespace UtilsQt
