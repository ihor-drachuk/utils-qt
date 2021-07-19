#include <utils-qt/qml/qml-controls.h>
#include <utils-qt/qml-cpp/ListModelItemProxy.h>
#include <utils-qt/qml-cpp/ListModelTools.h>
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
