/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <UtilsQt/Futures/SignalToFuture.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>

class Futures_SignalToFuture_TestClass : public QObject
{
    Q_OBJECT
public:
    void trigger1() { emit someSignal1(); }
    void trigger2() { emit someSignal2("Test"); }
    void trigger3() { emit someSignal3("Test", 12); }

signals:
    void someSignal1();
    void someSignal2(const QString& text);
    void someSignal3(const QString& text, int number);
};

using namespace UtilsQt;

TEST(UtilsQt, Futures_SignalToFuture_Basic)
{
    using Class = Futures_SignalToFuture_TestClass;

    Class obj;
    auto f = signalToFuture(&obj, &Class::someSignal1);

    qApp->processEvents();
    qApp->processEvents();
    ASSERT_FALSE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());

    obj.trigger2();
    obj.trigger3();
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_FALSE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());

    obj.trigger1();
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());

    auto f2 = signalToFuture(&obj, &Class::someSignal2);
    auto f3 = signalToFuture(&obj, &Class::someSignal3);
    obj.trigger2();
    obj.trigger3();
    ASSERT_TRUE(f2.isFinished());
    ASSERT_TRUE(f3.isFinished());
    ASSERT_FALSE(f2.isCanceled());
    ASSERT_FALSE(f3.isCanceled());
}


TEST(UtilsQt, Futures_SignalToFuture_Context)
{
    using Class = Futures_SignalToFuture_TestClass;

    // Main context (source object)
    auto obj = std::make_shared<Class>();
    auto f = signalToFuture(obj.get(), &Class::someSignal1);

    obj.reset();
    waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());

    // Usual context
    obj = std::make_shared<Class>();
    auto obj2 = std::make_shared<QObject>();
    f = signalToFuture(obj.get(), &Class::someSignal1, obj2.get());
    ASSERT_FALSE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());

    obj2.reset();
    waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());
}

TEST(UtilsQt, Futures_SignalToFuture_ReturnedValue)
{
    using Class = Futures_SignalToFuture_TestClass;

    Class obj;
    auto f1 = signalToFuture(&obj, &Class::someSignal1);
    auto f2 = signalToFuture(&obj, &Class::someSignal2);
    auto f3 = signalToFuture(&obj, &Class::someSignal3);

    static_assert (std::is_same_v<decltype(f1), QFuture<void>>, "Wrong return type for case 1");
    static_assert (std::is_same_v<decltype(f2), QFuture<QString>>, "Wrong return type for case 2");
    static_assert (std::is_same_v<decltype(f3), QFuture<std::tuple<QString, int>>>, "Wrong return type for case 3");

    obj.trigger1();
    obj.trigger2();
    obj.trigger3();

    waitForFuture<QEventLoop>(f1);
    waitForFuture<QEventLoop>(f2);
    waitForFuture<QEventLoop>(f3);

    ASSERT_TRUE(f1.isFinished());
    ASSERT_TRUE(f2.isFinished());
    ASSERT_TRUE(f3.isFinished());

    ASSERT_FALSE(f1.isCanceled());
    ASSERT_FALSE(f2.isCanceled());
    ASSERT_FALSE(f3.isCanceled());

    // Skip f1 (void)
    const auto r2 = f2.result();
    const auto r3 = f3.result();

    ASSERT_EQ(r2, "Test");
    ASSERT_EQ(r3, std::make_tuple(QString("Test"), 12));
}

TEST(UtilsQt, Futures_SignalToFuture_Timeout)
{
    using Class = Futures_SignalToFuture_TestClass;
    Class obj;

    // Timeout trigger
    auto f = signalToFuture(&obj, &Class::someSignal1, nullptr, 100);

    qApp->processEvents();
    qApp->processEvents();
    ASSERT_FALSE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());

    waitForFuture<QEventLoop>(createTimedFuture(200));

    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());

    // Timeout after signal
    f = signalToFuture(&obj, &Class::someSignal1, nullptr, 100);
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_FALSE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    obj.trigger1();
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());

    waitForFuture<QEventLoop>(createTimedFuture(200));

    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
}

#include "Test_Futures_5_SignalToFuture.moc"
