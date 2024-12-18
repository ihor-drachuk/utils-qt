/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <any>
#include <memory>
#include <type_traits>
#include <utils-cpp/default_ctor_ops.h>

/*              Description
-------------------------------------------
Usage example:
  /
  | auto ds1 = MakeSetterWithDeferredSignal(state, State::Connecting, this, &VpnController_L2::stateChanged);
  | auto ds2 = MakeSetterWithDeferredSignal(error, ErrorCode::NoError, this, &VpnController_L2::errorChanged);
  \

This code will first set all values and then emit all signals.
*/

template<typename T, typename T2, typename Signal>
class SetterWithDeferredSignal
{
    NO_COPY(SetterWithDeferredSignal);
public:
    SetterWithDeferredSignal(T& oldValue, const T2& newValue, const Signal& signal, bool* changedFlag = nullptr)
        : m_value(newValue),
          m_signal(signal)
    {
        m_isChanged = (oldValue != newValue);

        if (m_isChanged) {
            oldValue = newValue;
            if (changedFlag) *changedFlag = true;
        }
    }

    SetterWithDeferredSignal(SetterWithDeferredSignal&& rhs) noexcept
        : m_value(std::move(rhs.m_value)),
          m_signal(std::move(rhs.m_signal)),
          m_isChanged(rhs.m_isChanged)
    {
        rhs.m_isChanged = false;
    }

    ~SetterWithDeferredSignal() {
        if (m_isChanged)
            m_signal(m_value);
    }

private:
    T m_value;
    Signal m_signal;
    bool m_isChanged {};
};

template<typename T, typename T2, typename Signal>
SetterWithDeferredSignal(T&, const T2&, const Signal&, bool*) -> SetterWithDeferredSignal<T, T2, Signal>;

template<typename T, typename T2, typename Signal>
SetterWithDeferredSignal(T&, const T2&, const Signal&) -> SetterWithDeferredSignal<T, T2, Signal>;

template<typename T, typename T2, typename Signal>
SetterWithDeferredSignal<T, T2, Signal> MakeSetterWithDeferredSignal(T& oldValue, const T2& newValue, const Signal& signal, bool* changedFlag = nullptr)
{
    return SetterWithDeferredSignal<T, T2, Signal>(oldValue, newValue, signal, changedFlag);
}

template<typename T, typename T2, typename Signal>
std::any MakeSetterWithDeferredSignalAny(T& oldValue, const T2& newValue, const Signal& signal, bool* changedFlag = nullptr)
{
    return std::any(std::make_shared<SetterWithDeferredSignal<T, T2, Signal>>(oldValue, newValue, signal, changedFlag));
}

template<typename T, typename T2, typename Object, typename Signal>
auto MakeSetterWithDeferredSignal(T& oldValue, const T2& newValue, Object* object, const Signal& signal, bool* changedFlag = nullptr)
{
   return SetterWithDeferredSignal(oldValue, newValue, [object, signal](const T2& x){ (object->*signal)(x); }, changedFlag);
}

template<typename T, typename T2, typename Object, typename Signal>
std::any MakeSetterWithDeferredSignalAny(T& oldValue, const T2& newValue, Object* object, const Signal& signal, bool* changedFlag = nullptr)
{
    auto ptr = new SetterWithDeferredSignal(oldValue, newValue, [object, signal](const T2& x){ (object->*signal)(x); }, changedFlag);
    using SetterType = std::remove_cv_t<std::remove_reference_t<decltype(*ptr)>>;
    return std::any(std::shared_ptr<SetterType>(ptr));
}
