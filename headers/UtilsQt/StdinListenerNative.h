/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <utils-cpp/stdin_listener_native.h>

namespace UtilsQt {

class StdinListenerNative : public QObject
{
    Q_OBJECT
public:
    StdinListenerNative(): listener([this](const std::string& line) { emit newLineArrived(QString::fromStdString(line)); }) { }

signals:
    void newLineArrived(const QString& line);

private:
    utils_cpp::StdinListenerNative listener;
};

} // namespace UtilsQt
