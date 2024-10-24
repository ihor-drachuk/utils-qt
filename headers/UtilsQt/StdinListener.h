/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <string>

#include <QObject>

#include <utils-cpp/pimpl.h>
#include <utils-cpp/stdin_listener.h>

namespace UtilsQt {

class StdinListener : public QObject
{
    Q_OBJECT
public:
    using EchoMode = utils_cpp::StdinListener::EchoMode;
    using NewLineMode = utils_cpp::StdinListener::NewLineMode;

    StdinListener(EchoMode echoMode = EchoMode::Auto,
                  NewLineMode newLineMode = NewLineMode::Any,
                  bool listenData = false,
                  bool listenLines = true);
    ~StdinListener() override;

signals:
    void newDataArrived(const QByteArray& data);
    void newLineArrived(const QString& line);

private:
    void onDataArrived(const char* buffer, size_t sz);
    void onLineArrived(const std::string& line);

private:
    DECLARE_PIMPL
};

} // namespace UtilsQt
