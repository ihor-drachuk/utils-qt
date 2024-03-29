/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

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
#include <utils-cpp/function_traits.h>
#include <utils-cpp/copy_move.h>

/*
            Description
  -------------------------------

  There is single function:
    QFuture convertFuture(context, future, flags, converter);

  Is used to convert QFuture<T1> to QFuture<T2>. For example: one async operation returns raw buffer,
  while intermediate module should return QFuture of parsed data based on the raw buffer.

  'flags' can be omitted.

  'context' controls lifetime. If 'context' is gone, resulting future will be canceled.
  If 'context' is nullptr, then resulting future will be canceled immediately.
  This behavior can be overridden by flag IgnoreNullContext.

  If QFuture<T1> should be converted to QFuture<T2>, then handler's declaration should look
  like this:
  /
  |  [](const T1& input) -> std::optional<T2> {
  |     //...
  |  }
  \
  Returning nullopt will cancel resulting future.


  Another example: we have QFuture<int> and want to get QFuture<std::string>.
  /
  |  auto f = QFuture<int>();
  |
  |  auto f2 = convertFuture<int, std::string>(this, f, [](int value) -> std::optional<std::string>
  |  {
  |      return std::to_string(value);
  |  });
  \

  So, converted future will be canceled if/when:
   - Source future canceled
   - Converter returned nothing / nullopt
   - Context destroyed (it controls converter lifetime)
   - Context is nullptr and flag IgnoreNullContext isn't set

  Converted future will return result:
   - IF/WHEN  Source future has result
   - THEN     Converter returned new result
   - WHILE    Context still alive (or it's nullptr + IgnoreNullContext set)

  Source futures will be canceled, if target future is canceled and vice-versa.
  So futures cancellation is transitive and bi-directional.

  Possible 'flags' values:
   - IgnoreNullContext      - don't cancel resulting future if nullptr is passed as a context.
*/

namespace UtilsQt {
enum class ConverterFlags
{
    IgnoreNullContext = 1 // Otherwise cancel on null context
};
} // namespace UtilsQt

inline UtilsQt::ConverterFlags operator| (UtilsQt::ConverterFlags a, UtilsQt::ConverterFlags b)
{
    return static_cast<UtilsQt::ConverterFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline int operator& (UtilsQt::ConverterFlags a, UtilsQt::ConverterFlags b)
{
    return static_cast<int>(a) & static_cast<int>(b);
}

namespace UtilsQt {
namespace FutureConverterInternal {

template<typename T>
struct TargetTypeExtractor
{
    using Type = typename function_traits<T>::return_type::value_type;
};

template<typename>
struct IsOptional : std::false_type {};

template<typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template<typename T, typename R, typename... Args>
auto fixConverterImpl(const T& func, std::function<R(Args...)>*)
{
    if constexpr (IsOptional<R>::value) {
        return func;
    } else {
        return [func](Args... args){
            if constexpr (sizeof...(Args)) {
                return std::optional(func(std::forward<Args...>(args...)));
            } else {
                return std::optional(func());
            }
        };
    }
}

template<typename T>
auto fixConverter(const T& func)
{
    using StdFunc = decltype(std::function(func));
    return fixConverterImpl(func, static_cast<StdFunc*>(nullptr));
}

template <typename SrcFutureType, typename Converter, typename DstFutureType>
bool convert(const QFuture<SrcFutureType>& srcFuture, const Converter& converter, QFutureInterface<DstFutureType>& futureInterface)
{
    const auto result = converter(srcFuture.result());

    if (result) {
        futureInterface.reportResult(*result);
        futureInterface.reportFinished();
    }

    return result.has_value();
}

template <typename Converter, typename DstFutureType>
bool convert(const QFuture<void>& /*srcFuture*/, const Converter& converter, QFutureInterface<DstFutureType>& futureInterface)
{
    const auto result = converter();

    if (result) {
        futureInterface.reportResult(*result);
        futureInterface.reportFinished();
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
    NO_COPY_MOVE(Context);
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

template<typename Source, typename Target = std::nullptr_t, typename Converter,
         typename FixedConverter = decltype (FutureConverterInternal::fixConverter(std::declval<Converter>())),
         typename SelectedTarget = std::conditional_t<
             std::is_same_v<Target, std::nullptr_t>,
             typename FutureConverterInternal::TargetTypeExtractor<FixedConverter>::Type,
             Target>>
[[nodiscard]] QFuture<SelectedTarget> convertFuture(QObject* context,
                              const QFuture<Source>& srcFuture,
                              ConverterFlags flags,
                              const Converter& converter)
{
    if (!context && !(flags & ConverterFlags::IgnoreNullContext)) {
        QFutureInterface<SelectedTarget> result;
        result.reportCanceled();
        result.reportFinished();
        return result.future();
    }

    auto ctx = new FutureConverterInternal::Context<Source, SelectedTarget>(context, srcFuture, FutureConverterInternal::fixConverter(converter));
    return ctx->targetFuture();
}

template<typename Source, typename Target = std::nullptr_t, typename Converter,
         typename FixedConverter = decltype (FutureConverterInternal::fixConverter(std::declval<Converter>())),
         typename SelectedTarget = std::conditional_t<
             std::is_same_v<Target, std::nullptr_t>,
             typename FutureConverterInternal::TargetTypeExtractor<FixedConverter>::Type,
             Target>>
[[nodiscard]] QFuture<SelectedTarget> convertFuture(QObject* context,
                              const QFuture<Source>& srcFuture,
                              const Converter& converter)
{
    return convertFuture<Source, Target>(context, srcFuture, {}, converter);
}

} // namespace UtilsQt
