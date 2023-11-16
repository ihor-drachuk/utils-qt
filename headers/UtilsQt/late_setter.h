/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <utils-cpp/copy_move.h>

template<typename T, typename Signal>
class LateSetter
{
    NO_COPY_MOVE(LateSetter);
public:
    LateSetter(T& oldValue, const T& newValue, const Signal& signal, bool* changedFlag = nullptr)
        : m_value(oldValue),
          m_signal(signal)
    {
        m_isChanged = (oldValue != newValue);

        if (m_isChanged) {
            oldValue = newValue;
            if (changedFlag) *changedFlag = true;
        }
    }

    ~LateSetter() {
        if (m_isChanged)
            m_signal(m_value);
    }

private:
    const T& m_value;
    Signal m_signal;
    bool m_isChanged {};
};

template<typename T, typename Signal>
LateSetter<T, Signal> MakeLateSetter(T& oldValue, const T& newValue, const Signal& signal, bool* changedFlag = nullptr)
{
    return LateSetter<T, Signal>(oldValue, newValue, signal, changedFlag);
}
