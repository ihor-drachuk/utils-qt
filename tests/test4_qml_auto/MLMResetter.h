/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QQmlEngine>
#include <utils-cpp/default_ctor_ops.h>
#include <UtilsQt/MergedListModel.h>

class MLMResetter : public QObject
{
    Q_OBJECT
    NO_COPY_MOVE(MLMResetter);

public:
    static void registerTypes()
    {
        qmlRegisterSingletonType<MLMResetter>("UtilsQt", 1, 0, "MLMResetter", [](QQmlEngine*, QJSEngine*) -> QObject* {
            return new MLMResetter();
        });
    }

    explicit MLMResetter(QObject* parent = nullptr)
        : QObject(parent)
    { }

    Q_INVOKABLE void setCustomResetter(MergedListModel* mlm)
    {
        mlm->registerCustomResetter("interest", [](int /*role*/, const QLatin1String& /*roleStr*/, int /*row*/, const QVariant& /*prevValue*/){
            return "I-DELETED";
        });

        mlm->registerCustomResetter([](int /*role*/, const QLatin1String& /*roleStr*/, int /*row*/, const QVariant& /*prevValue*/){
            return "DELETED";
        });
    }
};
