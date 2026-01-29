/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/ValueSequenceWatcher.h>
#include <UtilsQt/qvariant_traits.h>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantMap>
#include <chrono>
#include <deque>
#include <optional>
#include <cassert>

namespace {

const char* const TypeKey = "_vsw_type";
const char* const ValueKey = "value";
const char* const MinMsKey = "minMs";
const char* const MaxMsKey = "maxMs";

struct SequenceElement {
    QVariant value;
    std::optional<int> optMinMs;
    std::optional<int> optMaxMs;
};

SequenceElement parseSequenceElement(const QVariant& element)
{
    SequenceElement result;

    if (element.canConvert<QVariantMap>()) {
        QVariantMap map = element.toMap();
        if (map.contains(TypeKey)) {
            result.value = map.value(ValueKey);

            if (map.contains(MinMsKey)) {
                int val = map.value(MinMsKey).toInt();
                assert(val > 0 && "MinDuration: ms must be > 0");
                result.optMinMs = val;
            }

            if (map.contains(MaxMsKey)) {
                int val = map.value(MaxMsKey).toInt();
                assert(val > 0 && "MaxDuration: ms must be > 0");
                result.optMaxMs = val;
            }

            if (result.optMinMs && result.optMaxMs) {
                assert(*result.optMinMs <= *result.optMaxMs && "Duration: minMs must be <= maxMs");
            }

            return result;
        }
    }

    // Plain value without duration constraints
    result.value = element;
    return result;
}

bool variantEquals(const QVariant& a, const QVariant& b)
{
    // Handle floating point comparison
    if (UtilsQt::QVariantTraits::isFloat(a) && UtilsQt::QVariantTraits::isFloat(b)) {
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    }

    return a == b;
}

} // namespace

struct ValueSequenceWatcher::impl_t
{
    struct HistoryEntry {
        QVariant value;
        std::chrono::steady_clock::time_point timestamp;
    };

    QVariant value;
    QVariantList sequence;
    bool enabled { true };
    bool once { false };
    bool resetOnTrigger { true };
    int triggerCount { 0 };
    bool alreadyTriggered { false };

    std::deque<HistoryEntry> history;
    QTimer holdTimer;
    bool componentCompleted { false };
};

void ValueSequenceWatcher::registerTypes()
{
    qmlRegisterType<ValueSequenceWatcher>("UtilsQt", 1, 0, "ValueSequenceWatcher");

    qmlRegisterSingletonType<VSW>("UtilsQt", 1, 0, "VSW",
        [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            auto obj = new VSW();
            engine->setObjectOwnership(obj, QQmlEngine::JavaScriptOwnership);
            return obj;
        });
}

ValueSequenceWatcher::ValueSequenceWatcher(QQuickItem* parent)
    : QQuickItem(parent)
{
    _impl = new impl_t();
    impl().holdTimer.setSingleShot(true);
    QObject::connect(&impl().holdTimer, &QTimer::timeout, this, &ValueSequenceWatcher::onTimerTimeout);
}

ValueSequenceWatcher::~ValueSequenceWatcher()
{
    delete _impl;
}

QVariant VSW::MinDuration(const QVariant& value, int ms)
{
    assert(ms > 0 && "MinDuration: ms must be > 0");

    QVariantMap result;
    result[TypeKey] = "MinDuration";
    result[ValueKey] = value;
    result[MinMsKey] = ms;
    return result;
}

QVariant VSW::MaxDuration(const QVariant& value, int ms)
{
    assert(ms > 0 && "MaxDuration: ms must be > 0");

    QVariantMap result;
    result[TypeKey] = "MaxDuration";
    result[ValueKey] = value;
    result[MaxMsKey] = ms;
    return result;
}

QVariant VSW::Duration(const QVariant& value, int minMs, int maxMs)
{
    assert(minMs > 0 && maxMs > 0 && minMs <= maxMs);

    QVariantMap result;
    result[TypeKey] = "Duration";
    result[ValueKey] = value;
    result[MinMsKey] = minMs;
    result[MaxMsKey] = maxMs;
    return result;
}

QVariant ValueSequenceWatcher::value() const
{
    return impl().value;
}

QVariantList ValueSequenceWatcher::sequence() const
{
    return impl().sequence;
}

bool ValueSequenceWatcher::enabled() const
{
    return impl().enabled;
}

bool ValueSequenceWatcher::once() const
{
    return impl().once;
}

bool ValueSequenceWatcher::resetOnTrigger() const
{
    return impl().resetOnTrigger;
}

int ValueSequenceWatcher::triggerCount() const
{
    return impl().triggerCount;
}

void ValueSequenceWatcher::setValue(const QVariant& value)
{
    if (impl().value == value)
        return;

    impl().value = value;
    emit valueChanged(impl().value);

    if (impl().componentCompleted) {
        onValueChanged();
    }
}

void ValueSequenceWatcher::setSequence(const QVariantList& sequence)
{
    if (impl().sequence == sequence)
        return;

    impl().sequence = sequence;
    emit sequenceChanged(impl().sequence);

    // Reset state when sequence changes
    if (impl().componentCompleted && impl().enabled) {
        impl().holdTimer.stop();
        resetHistory();
        addToHistory(impl().value);
        checkMatch();
    }
}

void ValueSequenceWatcher::setEnabled(bool enabled)
{
    if (impl().enabled == enabled)
        return;

    impl().enabled = enabled;
    emit enabledChanged(impl().enabled);

    if (impl().componentCompleted) {
        if (enabled) {
            // enabled: false -> true: reset history and start fresh
            resetHistory();
            addToHistory(impl().value);
            checkMatch();
        } else {
            // enabled: true -> false: stop timer
            impl().holdTimer.stop();
        }
    }
}

void ValueSequenceWatcher::setOnce(bool once)
{
    if (impl().once == once)
        return;

    impl().once = once;
    emit onceChanged(impl().once);
}

void ValueSequenceWatcher::setResetOnTrigger(bool resetOnTrigger)
{
    if (impl().resetOnTrigger == resetOnTrigger)
        return;

    impl().resetOnTrigger = resetOnTrigger;
    emit resetOnTriggerChanged(impl().resetOnTrigger);
}

void ValueSequenceWatcher::componentComplete()
{
    QQuickItem::componentComplete();
    impl().componentCompleted = true;

    if (impl().enabled) {
        resetHistory();
        addToHistory(impl().value);
        checkMatch();
    }
}

void ValueSequenceWatcher::onValueChanged()
{
    if (!impl().enabled)
        return;

    if (impl().once && impl().alreadyTriggered)
        return;

    // Stop any pending timer since value changed
    impl().holdTimer.stop();

    addToHistory(impl().value);
    checkMatch();
}

void ValueSequenceWatcher::onTimerTimeout()
{
    if (!impl().enabled)
        return;

    if (impl().once && impl().alreadyTriggered)
        return;

    // Timer fired, meaning MinDuration has been satisfied
    // Do final check and trigger if still matching
    const auto& history = impl().history;
    const auto& sequence = impl().sequence;

    if (history.empty() || sequence.isEmpty())
        return;

    const int seqLen = sequence.size();
    const int histLen = static_cast<int>(history.size());

    if (histLen < seqLen)
        return;

    // Get the last element requirements
    SequenceElement lastSeqElem = parseSequenceElement(sequence.last());

    // Check if current value still matches
    const auto& lastHistEntry = history.back();
    if (!variantEquals(lastHistEntry.value, lastSeqElem.value))
        return;

    // Check MaxDuration for last element if specified
    if (lastSeqElem.optMaxMs) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHistEntry.timestamp).count();
        if (elapsed > *lastSeqElem.optMaxMs)
            return;
    }

    doTrigger();
}

void ValueSequenceWatcher::resetHistory()
{
    impl().history.clear();
    impl().holdTimer.stop();
}

void ValueSequenceWatcher::addToHistory(const QVariant& value)
{
    impl_t::HistoryEntry entry;
    entry.value = value;
    entry.timestamp = std::chrono::steady_clock::now();
    impl().history.push_back(entry);

    // Keep history size bounded to sequence length
    const auto maxLen = static_cast<size_t>(impl().sequence.size());
    while (impl().history.size() > maxLen && maxLen > 0) {
        impl().history.pop_front();
    }
}

void ValueSequenceWatcher::checkMatch()
{
    const auto& history = impl().history;
    const auto& sequence = impl().sequence;

    if (sequence.isEmpty())
        return;

    const int seqLen = sequence.size();
    const int histLen = static_cast<int>(history.size());

    if (histLen < seqLen)
        return;

    auto now = std::chrono::steady_clock::now();

    // Check all elements except the last one (they need to match value and MaxDuration)
    for (int i = 0; i < seqLen - 1; ++i) {
        const int histIdx = histLen - seqLen + i;
        const auto& histEntry = history[histIdx];
        SequenceElement seqElem = parseSequenceElement(sequence[i]);

        // Check value match
        if (!variantEquals(histEntry.value, seqElem.value))
            return;

        // For non-last elements, check MaxDuration (value must not have stayed longer than max)
        if (seqElem.optMaxMs) {
            // Calculate how long this value was held (until next value arrived)
            const auto& nextHistEntry = history[histIdx + 1];
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                nextHistEntry.timestamp - histEntry.timestamp).count();
            if (duration > *seqElem.optMaxMs)
                return;
        }

        // For non-last elements, check MinDuration (value must have stayed at least min time)
        if (seqElem.optMinMs) {
            const auto& nextHistEntry = history[histIdx + 1];
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                nextHistEntry.timestamp - histEntry.timestamp).count();
            if (duration < *seqElem.optMinMs)
                return;
        }
    }

    // Check the last element
    const auto& lastHistEntry = history.back();
    SequenceElement lastSeqElem = parseSequenceElement(sequence.last());

    // Check value match
    if (!variantEquals(lastHistEntry.value, lastSeqElem.value))
        return;

    // For the last element, check how long it has been held
    auto lastDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastHistEntry.timestamp).count();

    // Check MaxDuration for last element
    if (lastSeqElem.optMaxMs && lastDuration > *lastSeqElem.optMaxMs)
        return;

    // Check MinDuration for last element
    if (lastSeqElem.optMinMs) {
        if (lastDuration >= *lastSeqElem.optMinMs) {
            // MinDuration already satisfied
            doTrigger();
        } else {
            // Need to wait for MinDuration
            int remainingMs = *lastSeqElem.optMinMs - static_cast<int>(lastDuration);
            impl().holdTimer.start(remainingMs);
        }
    } else {
        // No MinDuration requirement, trigger immediately
        doTrigger();
    }
}

void ValueSequenceWatcher::doTrigger()
{
    if (impl().once && impl().alreadyTriggered)
        return;

    impl().alreadyTriggered = true;
    setTriggerCount(impl().triggerCount + 1);
    emit triggered();

    if (impl().resetOnTrigger) {
        resetHistory();
        // Add current value as first element for potential future matches
        addToHistory(impl().value);
    }
    // If resetOnTrigger is false, history is kept (sliding window behavior)
}

void ValueSequenceWatcher::setTriggerCount(int count)
{
    if (impl().triggerCount == count)
        return;

    impl().triggerCount = count;
    emit triggerCountChanged(impl().triggerCount);
}
