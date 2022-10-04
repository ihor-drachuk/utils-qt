#include <UtilsQt/Qml/qml-controls.h>
#include <UtilsQt/Qml-Cpp/ListModelItemProxy.h>
#include <UtilsQt/Qml-Cpp/ListModelTools.h>
#include <QDir>
#include <QQmlEngine>

inline void initResource() {
    Q_INIT_RESOURCE(qml_controls);
}

namespace QmlControls {


void init(QQmlEngine& qmlEngine) {
    initResource();

    qmlEngine.addImportPath(":/utils-qt");
    qmlEngine.addImportPath("qrc:/utils-qt");

    ListModelItemProxy::registerTypes();
    ListModelTools::registerTypes();
}

} // namespace QmlModules
