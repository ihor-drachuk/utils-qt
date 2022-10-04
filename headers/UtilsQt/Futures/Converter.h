#pragma once
#include <functional>
#include <optional>
#include <QObject>
#include <QSharedPointer>
#include <QFuture>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QPair>
#include <utils-cpp/pimpl.h>

/*            Reminder
  -------------------------------

  // Example: we have QFuture<int> and want to get QFuture<std::string>.

  auto f = QFuture<int>();

  auto f2 = convertFuture<int, std::string>(this, f, [](int value) -> std::optional<std::string>
  {
      return std::to_string(value);
  });

  -------------------------------

  Converted future will be canceled if/when:
   - Source future canceled
   - Converter returned nothing / nullopt
   - Context destroyed (it controls converter lifetime)

  Converted future will return result:
   - IF/WHEN  Source future has result
   - THEN     Converter returned new result
   - WHILE    Context still alive

  You can cancel source future by cancelling target future.
  The same is done if context destroyed.
*/

namespace UtilsQt {
namespace FutureConverterInternal {


template <typename SrcFutureType, typename Converter, typename DstFutureType>
bool convert(const QFuture<SrcFutureType>& srcFuture, const Converter& converter, QFutureInterface<DstFutureType>& interface)
{
    const auto result = converter(srcFuture.result());

    if (result) {
        interface.reportResult(*result);
        interface.reportFinished();
    }

    return result.has_value();
}

template <typename Converter, typename DstFutureType>
bool convert(const QFuture<void>& /*srcFuture*/, const Converter& converter, QFutureInterface<DstFutureType>& interface)
{
    const auto result = converter();

    if (result) {
        interface.reportResult(*result);
        interface.reportFinished();
    }

    return result.has_value();
}

template<typename Source, typename Target>
struct ConverterType
{
    using Converter = std::function<std::optional<Target>(const Source&)>;
};

template<typename Target>
struct ConverterType<void, Target>
{
    using Converter = std::function<std::optional<Target>()>;
};

template<typename Source, typename Target>
class Context : public QObject
{
public:
    using SourceFuture = QFuture<Source>;
    using SourceWatcher = QFutureWatcher<Source>;
    using TargetWatcher = QFutureWatcher<Target>;
    using Converter = typename ConverterType<Source, Target>::Converter;

    Context(QObject* context, const SourceFuture& future, const Converter& converter)
        : QObject(context),
          m_converter(converter)
    {
        if (future.isStarted())
            m_targetFutureInterface.reportStarted();

        if (future.isCanceled())
            m_targetFutureInterface.reportCanceled();

        QObject::connect(&m_targetWatcher, &TargetWatcher::canceled, [this](){ m_sourceWatcher.future().cancel(); });
        m_targetWatcher.setFuture(m_targetFutureInterface.future());

        if (future.isFinished()) {
            handleFinished(future);
        } else {
            QObject::connect(&m_sourceWatcher, &SourceWatcher::started, [this](){ m_targetFutureInterface.reportStarted(); });
            QObject::connect(&m_sourceWatcher, &SourceWatcher::canceled, [this](){ m_targetFutureInterface.reportCanceled(); });
            QObject::connect(&m_sourceWatcher, &SourceWatcher::finished, [this, future](){ handleFinished(future); });
            m_sourceWatcher.setFuture(future);
        }
    }

    ~Context() override {
        if (!m_targetFutureInterface.isFinished())
            doCancel();
    }

    QFuture<Target> targetFuture() { return m_targetFutureInterface.future(); };

private:
    void handleFinished(const SourceFuture& future) {
        if (future.isCanceled()) {
            doCancel();
        } else {
            auto convertedResult = convert(future, m_converter, m_targetFutureInterface);

            if (!convertedResult)
                doCancel();
        }

        deleteLater();
    }

    void doCancel() {
        if (!m_sourceWatcher.future().isCanceled())
            m_sourceWatcher.future().cancel();

        if (!m_targetFutureInterface.isCanceled())
            m_targetFutureInterface.reportCanceled();

        m_targetFutureInterface.reportFinished();
    }

private:
    Converter m_converter;
    QFutureInterface<Target> m_targetFutureInterface;
    SourceWatcher m_sourceWatcher;
    TargetWatcher m_targetWatcher;
};

} // namespace FutureConverterInternal


template<typename Source, typename Target, typename Converter>
QFuture<Target> convertFuture(QObject* context,
                              const QFuture<Source>& srcFuture,
                              const Converter& converter)
{
    auto ctx = new FutureConverterInternal::Context<Source, Target>(context, srcFuture, converter);
    return ctx->targetFuture();
}

} // namespace UtilsQt
