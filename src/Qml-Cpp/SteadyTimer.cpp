/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/SteadyTimer.h>
#include <QTimer>
#include <QQmlEngine>

struct SteadyTimer::impl_t
{
    QTimer timer;
    std::chrono::steady_clock::time_point nextTimepoint;

    int interval { 1000 };
    bool repeat { false };
    int resolution { 250 };
    bool active { false };
    double thresholdFactor { 0.1 };
    bool manuallySetResolution { false };
};

void SteadyTimer::registerTypes()
{
    qmlRegisterType<SteadyTimer>("UtilsQt", 1, 0, "SteadyTimer");
}

SteadyTimer::SteadyTimer(QObject* parent)
    : QObject(parent)
{
    createImpl();
    impl().timer.setSingleShot(false);
    impl().timer.setInterval(impl().resolution);
    QObject::connect(&impl().timer, &QTimer::timeout, this, &SteadyTimer::onTriggered);
}

SteadyTimer::~SteadyTimer() = default;

void SteadyTimer::start()
{
    stop();
    setActive(true);
}

void SteadyTimer::start(const std::optional<int>& optInterval, const std::optional<bool>& optRepeat)
{
    if (optRepeat)
        setRepeat(*optRepeat);

    if (optInterval)
        setInterval(*optInterval);

    start();
}

void SteadyTimer::start(std::chrono::milliseconds interval, const std::optional<bool>& optRepeat)
{
    start(interval.count(), optRepeat);
}

void SteadyTimer::start(std::chrono::steady_clock::time_point timepoint)
{
    start(std::chrono::duration_cast<std::chrono::milliseconds>(timepoint - std::chrono::steady_clock::now()), false);
}

void SteadyTimer::stop()
{
    setActive(false);
}

int SteadyTimer::interval() const
{
    return impl().interval;
}

void SteadyTimer::setInterval(int newValue)
{
    assert(newValue >= 0);

    if (impl().interval == newValue)
        return;

    impl().interval = newValue;
    emit intervalChanged(impl().interval);

    if (active())
        start();
}

void SteadyTimer::setInterval(std::chrono::milliseconds interval)
{
    setInterval(interval.count());
}

bool SteadyTimer::repeat() const
{
    return impl().repeat;
}

void SteadyTimer::setRepeat(bool newValue)
{
    if (impl().repeat == newValue)
        return;
    impl().repeat = newValue;
    emit repeatChanged(impl().repeat);
}

int SteadyTimer::resolution() const
{
    return impl().resolution;
}

void SteadyTimer::setResolution(int newValue)
{
    assert(newValue > 0);

    if (impl().resolution == newValue)
        return;
    impl().resolution = newValue;
    impl().timer.setInterval(impl().resolution);
    impl().manuallySetResolution = true;
    emit resolutionChanged(impl().resolution);
}

void SteadyTimer::setResolution(std::chrono::milliseconds resolution)
{
    setResolution(resolution.count());
}

bool SteadyTimer::active() const
{
    return impl().active;
}

void SteadyTimer::setActive(bool newValue)
{
    if (impl().active == newValue)
        return;

    if (newValue && impl().interval == 0) {
        assert(!impl().repeat);

        impl().active = true;
        emit activeChanged(impl().active);

        impl().active = false;
        emit activeChanged(impl().active);

        QMetaObject::invokeMethod(this, "timeout", Qt::QueuedConnection);
        return;
    }

    impl().active = newValue;

    if (newValue) {
        if (!impl().manuallySetResolution) {
            auto newResolution = impl().resolution;

            if (impl().resolution >= impl().interval) {
                newResolution = impl().interval;

            } else if (impl().resolution < 250) {
                newResolution = std::min(250, impl().interval);
            }

            if (newResolution != impl().resolution) {
                impl().resolution = newResolution;
                impl().timer.setInterval(impl().resolution);
                emit resolutionChanged(impl().resolution);
            }
        }

        assert(impl().resolution <= impl().interval);
        impl().nextTimepoint = std::chrono::steady_clock::now() + std::chrono::milliseconds(impl().interval);
        impl().timer.start();

    } else {
        impl().timer.stop();
    }

    emit activeChanged(impl().active);
}

void SteadyTimer::onTriggered()
{
    if (!impl().active)
        return;

    const auto threshold = std::chrono::duration<double, std::milli>(impl().resolution) * impl().thresholdFactor;

    if (impl().nextTimepoint - std::chrono::steady_clock::now() <= threshold) {
        if (impl().repeat) {
            impl().nextTimepoint += std::chrono::milliseconds(impl().interval);
        } else {
            setActive(false);
        }

        emit timeout();
    }
}

double SteadyTimer::thresholdFactor() const
{
    return impl().thresholdFactor;
}

void SteadyTimer::setThresholdFactor(double newValue)
{
    if (qFuzzyCompare(impl().thresholdFactor, newValue))
        return;
    impl().thresholdFactor = newValue;
    emit thresholdFactorChanged(impl().thresholdFactor);
}
