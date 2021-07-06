#include <QtQuickTest>
#include <utils-qt/qml.h>

class Setup : public QObject
{
    Q_OBJECT
public:
    Setup() {
    }

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
        UtilsQt::Qml::registerAll(*engine);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(test_utils_qt_4, Setup);

#include "main4.moc"
