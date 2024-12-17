/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <optional>
#include <chrono>
#include <utils-cpp/pimpl.h>

class SteadyTimer : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged FINAL)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY repeatChanged FINAL)
    Q_PROPERTY(int resolution READ resolution WRITE setResolution NOTIFY resolutionChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(double thresholdFactor READ thresholdFactor WRITE setThresholdFactor NOTIFY thresholdFactorChanged FINAL)

    static void registerTypes();

    explicit SteadyTimer(QObject* parent = nullptr);
    ~SteadyTimer() override;

    Q_INVOKABLE void start();
    void start(const std::optional<int>& optInterval, const std::optional<bool>& optRepeat = {});
    void start(std::chrono::milliseconds interval, const std::optional<bool>& optRepeat = {});
    void start(std::chrono::steady_clock::time_point timepoint); // Sets 'repeat' to 'false'
    Q_INVOKABLE void stop();

    template<typename Object, typename Signal>
    auto callOnTimeout(Object* object, Signal signal, Qt::ConnectionType connectionType = Qt::ConnectionType::AutoConnection) {
        return connect(this, &SteadyTimer::timeout, object, signal, connectionType);
    }

signals:
    void timeout();

    // -- Properties support --
public:
    int interval() const;
    void setInterval(int newValue);
    void setInterval(std::chrono::milliseconds interval);

    bool repeat() const;
    void setRepeat(bool newValue);

    int resolution() const;
    void setResolution(int newValue);
    void setResolution(std::chrono::milliseconds resolution);

    bool active() const;
    void setActive(bool newValue);

    double thresholdFactor() const;
    void setThresholdFactor(double newValue);

signals:
    void intervalChanged(int interval);
    void repeatChanged(bool repeat);
    void resolutionChanged(int resolution);
    void activeChanged(bool active);
    // -- --

    void thresholdFactorChanged(double thresholdFactor);

private:
    void onTriggered();

private:
    DECLARE_PIMPL
};
