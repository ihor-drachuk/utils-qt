#pragma once

template<typename T, typename Signal>
class LateSetter
{
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
