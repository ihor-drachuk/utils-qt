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
    Q_PROPERTY(int value2 READ value2 /*WRITE setValue2*/ NOTIFY value2Changed FINAL)

    int value() const { return m_value; }
    int value2() const { return m_value2; }

    std::any setValue(int newValue)
    {
        auto ds = setValue2(newValue * 2);
        return MakeSetterWithDeferredSignalAny(m_value, newValue, this, &SetterWithDeferredSignal_TestClass::valueChanged, nullptr, {ds});
    }

signals:
    void valueChanged(int value);
    void value2Changed(int value);

private:
    std::any setValue2(int newValue)
    {
        return MakeSetterWithDeferredSignalAny(m_value2, newValue, this, &SetterWithDeferredSignal_TestClass::value2Changed);
    }

private:
    int m_value { 0 };
    int m_value2 { 0 };
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

TEST(UtilsQt, SetterWithDeferredSignal_Adjuncts)
{
    SetterWithDeferredSignal_TestClass tc;

    QSignalSpy spy1(&tc, &SetterWithDeferredSignal_TestClass::valueChanged);
    QSignalSpy spy2(&tc, &SetterWithDeferredSignal_TestClass::value2Changed);

    {
        auto ds1 = tc.setValue(1); // also sets value2
        ASSERT_EQ(tc.value(), 1);
        ASSERT_EQ(tc.value2(), 2);

        ASSERT_EQ(spy1.count(), 0);
        ASSERT_EQ(spy2.count(), 0);
    }

    ASSERT_EQ(spy1.count(), 1);
    ASSERT_EQ(spy2.count(), 1);
    ASSERT_EQ(spy1.at(0).at(0).toInt(), 1);
    ASSERT_EQ(spy2.at(0).at(0).toInt(), 2);
}

#include "test14_SetterWithDeferredSignal.moc"
