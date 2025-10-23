/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/FilterBehavior.h>
#include <QQmlEngine>
#include <QTimer>
#include <optional>
#include <private/qqmlproperty_p.h>

struct FilterBehavior::impl_t
{
    QQmlProperty property;
    int delay { 0 };
    bool when { true };
    std::optional<QVariant> optPendingValue;
    QTimer timer;
};

void FilterBehavior::registerTypes()
{
    qmlRegisterType<FilterBehavior>("UtilsQt", 1, 0, "FilterBehavior");
}

FilterBehavior::FilterBehavior(QObject* parent)
    : QObject(parent)
{
    _impl = new impl_t();
    impl().timer.setSingleShot(true);
    impl().timer.callOnTimeout(this, &FilterBehavior::onTimerTimeout);
}

FilterBehavior::~FilterBehavior()
{
    delete _impl;
}

void FilterBehavior::setTarget(const QQmlProperty& property)
{
    impl().property = property;
}

void FilterBehavior::write(const QVariant& value)
{
    impl().optPendingValue = value;

    QTimer::singleShot(0, this, [this]() {
        if (!impl().optPendingValue) return;

        auto value = *impl().optPendingValue;
        impl().optPendingValue.reset();

        if (impl().when && impl().delay > 0) {
            impl().optPendingValue = value;
            impl().timer.start(impl().delay);
        } else {
            impl().timer.stop();
            applyValue(value);
        }
    });
}

int FilterBehavior::delay() const
{
    return impl().delay;
}

bool FilterBehavior::when() const
{
    return impl().when;
}

void FilterBehavior::setDelay(int delay)
{
    if (impl().delay == delay)
        return;

    impl().delay = delay;
    emit delayChanged(impl().delay);
}

void FilterBehavior::setWhen(bool when)
{
    if (impl().when == when)
        return;

    impl().when = when;
    emit whenChanged(impl().when);
}

void FilterBehavior::onTimerTimeout()
{
    if (impl().optPendingValue) {
        applyValue(*impl().optPendingValue);
        impl().optPendingValue.reset();
    }
}

void FilterBehavior::applyValue(const QVariant& value)
{
    QQmlPropertyPrivate::write(impl().property, value,
                               QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
}
