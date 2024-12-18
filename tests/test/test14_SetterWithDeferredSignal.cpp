/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <QObject>
#include <QEventLoop>
#include <QSignalSpy>
#include <UtilsQt/SetterWithDeferredSignal.h>

class SetterWithDeferredSignal_TestClass : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged FINAL)

    int value() const { return m_value; }

    std::any setValue(int newValue)
    {
        return MakeSetterWithDeferredSignalAny(m_value, newValue, this, &SetterWithDeferredSignal_TestClass::valueChanged);
    }

signals:
    void valueChanged(int value);

private:
    int m_value { 0 };
};

TEST(UtilsQt, SetterWithDeferredSignal_Basic)
{
    SetterWithDeferredSignal_TestClass tc;
    QSignalSpy spy(&tc, &SetterWithDeferredSignal_TestClass::valueChanged);

    tc.setValue(0);
    ASSERT_EQ(spy.count(), 0);

    tc.setValue(123);
    ASSERT_EQ(spy.count(), 1);
    ASSERT_EQ(tc.value(), 123);
    ASSERT_EQ(spy.at(0).at(0).toInt(), 123);
}

TEST(UtilsQt, SetterWithDeferredSignal_Deferred)
{
    SetterWithDeferredSignal_TestClass tc;
    QSignalSpy spy(&tc, &SetterWithDeferredSignal_TestClass::valueChanged);

    {
        auto ds1 = tc.setValue(1);
        ASSERT_EQ(tc.value(), 1);

        auto ds2 = tc.setValue(2);
        ASSERT_EQ(tc.value(), 2);

        ASSERT_EQ(spy.count(), 0);
    }

    ASSERT_EQ(spy.count(), 2);
    ASSERT_EQ(tc.value(), 2);
    ASSERT_EQ(spy.at(0).at(0).toInt(), 2); // Notice! Order is reversed!
    ASSERT_EQ(spy.at(1).at(0).toInt(), 1);
}

#include "test14_SetterWithDeferredSignal.moc"
