/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/StdinListener.h>

namespace UtilsQt {

struct StdinListener::impl_t
{
    template<typename F1, typename F2>
    impl_t(EchoMode echoMode, NewLineMode newLineMode, const F1& dataHandler, const F2& lineHandler)
        : listener(echoMode, newLineMode, dataHandler, lineHandler)
    { }

    utils_cpp::StdinListener listener;
};

StdinListener::StdinListener(EchoMode echoMode, NewLineMode newLineMode, bool listenData, bool listenLines)
{
    using namespace std::placeholders;

    createImpl(echoMode,
               newLineMode,
               listenData ? std::bind(&StdinListener::onDataArrived, this, _1, _2) : utils_cpp::StdinListener::NewDataHandler(),
               listenLines ? std::bind(&StdinListener::onLineArrived, this, _1) : utils_cpp::StdinListener::NewLineHandler());
}

StdinListener::~StdinListener() = default;

void StdinListener::onDataArrived(const char* buffer, size_t sz)
{
    emit newDataArrived(QByteArray(buffer, static_cast<int>(sz)));
}

void StdinListener::onLineArrived(const std::string& line)
{
    emit newLineArrived(QString::fromStdString(line));
}

} // namespace UtilsQt
