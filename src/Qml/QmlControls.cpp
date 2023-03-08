#include <UtilsQt/Qml/QmlControls.h>
#include <UtilsQt/Qml-Cpp/ListModelItemProxy.h>
#include <UtilsQt/Qml-Cpp/ListModelTools.h>
#include <QQmlEngine>

inline void initResource() {
    Q_INIT_RESOURCE(QmlControls);
}

namespace QmlControls {

void init(QQmlEngine& qmlEngine) {
    initResource();

    qmlEngine.addImportPath(":/UtilsQt");
    qmlEngine.addImportPath("qrc:/UtilsQt");

    ListModelItemProxy::registerTypes();
    ListModelTools::registerTypes();
}

} // namespace QmlModules
