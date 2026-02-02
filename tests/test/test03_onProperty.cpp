/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <UtilsQt/OnProperty.h>
#include <UtilsQt/Futures/Utils.h>
#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>

using namespace UtilsQt;

class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int counter READ counter WRITE setCounter NOTIFY counterChanged)
public:
    TestObject(bool binary = false) {
        m_timer.setInterval(100);
        m_timer.setTimerType(Qt::TimerType::PreciseTimer);
        QObject::connect(&m_timer, &QTimer::timeout, this, [this, binary](){
            int newValue = counter() + 1;
            setCounter( binary ? (newValue % 2) : newValue );
        });
        m_timer.start();
    }

public:
    int counter() const { return m_counter; }
    void stop() { m_timer.stop(); }

public slots:
    void setCounter(int counter)
    {
        if (m_counter == counter)
            return;

        m_counter = counter;
        emit counterChanged(m_counter);
        emit counterChanged2();
        emit counterChanged3(m_counter, {});
    }

signals:
    void counterChanged(const int& counter);
    void counterChanged2();
    void counterChanged3(int, float);

private:
    int m_counter { 0 };
    QTimer m_timer;
};


TEST(UtilsQt, onProperty_initial)
{
    {
        int triggeredCount = 0;
        int cancelledCount = 0;
        QObject context;
        TestObject testObject;

        onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 0, UtilsQt::Comparison::Equal, true, &context, [&]()
        {
            triggeredCount++;
        },
        100,
        [&](auto){ cancelledCount++; });

        ASSERT_EQ(triggeredCount, 1);
        ASSERT_EQ(cancelledCount, 0);
    }

    {
        int triggeredCount = 0;
        QObject context;
        TestObject testObject;

        onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 1, UtilsQt::Comparison::NotEqual, true, &context, [&]()
        {
            triggeredCount++;
        });

        ASSERT_EQ(triggeredCount, 1);
    }

    {
        int triggeredCount = 0;
        QObject context;
        TestObject testObject;

        onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 0, UtilsQt::Comparison::NotEqual, true, &context, [&]()
        {
            triggeredCount++;
        });

        ASSERT_EQ(triggeredCount, 0);
    }
}


TEST(UtilsQt, onProperty_once)
{
    {
        int triggeredCount = 0;
        int cancelledCount = 0;
        QObject context;
        QElapsedTimer elapsedTimer;

        TestObject testObject;
        elapsedTimer.start();

        QEventLoop loop;

        onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 4, UtilsQt::Comparison::Equal, true, &context, [&]()
        {
            triggeredCount++;
            loop.quit();
        },
        700,
        [&](auto){ cancelledCount++; });
        loop.exec();

        ASSERT_EQ(triggeredCount, 1);
        ASSERT_EQ(cancelledCount, 0);
        ASSERT_EQ(testObject.counter(), 4);

        // Timer fires every 100ms, counter reaches 4 after ~400ms
        // Allow generous timing tolerance for CI environments
        auto elapsed = elapsedTimer.elapsed();
        ASSERT_GE(elapsed, 300) << "Triggered too early";
        ASSERT_LE(elapsed, 2000) << "Triggered too late";

        // Wait a bit more and verify no additional triggers
        waitForFuture<QEventLoop>(createTimedFuture(200));
        ASSERT_EQ(triggeredCount, 1);
        ASSERT_EQ(cancelledCount, 0);
    }

    {
        int triggeredCount = 0;
        QObject context;
        QElapsedTimer elapsedTimer;

        TestObject testObject;
        elapsedTimer.start();

        QEventLoop loop;

        onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 4, UtilsQt::Comparison::Equal, false, &context, [&]()
        {
            triggeredCount++;
            loop.quit();
        });
        loop.exec();

        ASSERT_EQ(triggeredCount, 1);
        ASSERT_EQ(testObject.counter(), 4);

        auto elapsed = elapsedTimer.elapsed();
        ASSERT_GE(elapsed, 300) << "Triggered too early";
        ASSERT_LE(elapsed, 2000) << "Triggered too late";

        // Wait a bit more and verify no additional triggers
        waitForFuture<QEventLoop>(createTimedFuture(200));
        ASSERT_EQ(triggeredCount, 1);
    }
}

TEST(UtilsQt, onProperty_multiple)
{
    int triggeredCount = 0;
    QObject* context = new QObject;
    TestObject testObject(true);

    onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 1, UtilsQt::Comparison::Equal, false, context, [&]()
    {
        triggeredCount++;
    });

    // Wait for multiple triggers (binary mode: toggles 0->1->0->1...)
    // Timer fires every 100ms, so in ~800ms we should see several triggers
    waitForFuture<QEventLoop>(createTimedFuture(800));

    // Should have triggered multiple times (at least 3 times in 800ms with 100ms interval)
    ASSERT_GE(triggeredCount, 2) << "Too few triggers";
    ASSERT_LE(triggeredCount, 10) << "Too many triggers";

    auto savedCount = triggeredCount;
    delete context; context = nullptr;

    // After context destruction, no more triggers
    waitForFuture<QEventLoop>(createTimedFuture(400));
    ASSERT_EQ(savedCount, triggeredCount);
}

TEST(UtilsQt, onProperty_cancelled)
{
    auto tester = [](bool stopTimer)
    {
        int triggeredCount = 0;
        UtilsQt::CancelReason reason = UtilsQt::CancelReason::Unknown;

        QObject context;
        QElapsedTimer elapsedTimer;

        TestObject testObject;
        if (stopTimer) testObject.stop();
        elapsedTimer.start();
        auto elapsed = elapsedTimer.elapsed();

        onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 1, UtilsQt::Comparison::Equal, true, &context, [&]()
        {
            triggeredCount++;
            elapsed = elapsedTimer.elapsed();
        },
        150,
        [&](UtilsQt::CancelReason value){
            reason = value;
            elapsed = elapsedTimer.elapsed();
        }
        );

        waitForFuture<QEventLoop>(createTimedFuture(500));

        if (stopTimer) {
            ASSERT_EQ(triggeredCount, 0);
            ASSERT_EQ(reason, UtilsQt::CancelReason::Timeout);

            // Timeout was set to 150ms
            ASSERT_GE(elapsed, 100) << "Timeout too early";
            ASSERT_LE(elapsed, 500) << "Timeout too late";
        } else {
            ASSERT_EQ(triggeredCount, 1);
            ASSERT_EQ(reason, UtilsQt::CancelReason::Unknown);
        }
    };

    tester(true);
    tester(false);
}


TEST(UtilsQt, onProperty_future)
{
    {
        QObject context;
        TestObject testObject;

        auto f = onPropertyFuture(&testObject, &TestObject::counter, &TestObject::counterChanged, 3, UtilsQt::Comparison::Equal, &context);

        ASSERT_FALSE(f.isFinished());
        QEventLoop loop;
        loop.processEvents();
        loop.processEvents();
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());

        ASSERT_EQ(testObject.counter(), 3);
    }

    {
        QObject context;
        TestObject testObject;

        auto f = onPropertyFuture(&testObject, &TestObject::counter, &TestObject::counterChanged, 3, UtilsQt::Comparison::Equal, &context, 20);

        ASSERT_FALSE(f.isFinished());
        QEventLoop loop;
        loop.processEvents();
        loop.processEvents();
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
    }
}

TEST(UtilsQt, onProperty_flexible_signal)
{
    QObject context;
    TestObject testObject;
    constexpr int expected = 3;

    auto f1 = onPropertyFuture(&testObject, &TestObject::counter, &TestObject::counterChanged,  expected, UtilsQt::Comparison::Equal, &context);
    auto f2 = onPropertyFuture(&testObject, &TestObject::counter, &TestObject::counterChanged2, expected, UtilsQt::Comparison::Equal, &context);
    auto f3 = onPropertyFuture(&testObject, &TestObject::counter, &TestObject::counterChanged3, expected, UtilsQt::Comparison::Equal, &context);

    waitForFuture<QEventLoop>(f1);
    waitForFuture<QEventLoop>(f2);
    waitForFuture<QEventLoop>(f3);

    ASSERT_TRUE(f1.isFinished());
    ASSERT_TRUE(f2.isFinished());
    ASSERT_TRUE(f3.isFinished());
    ASSERT_FALSE(f1.isCanceled());
    ASSERT_FALSE(f2.isCanceled());
    ASSERT_FALSE(f3.isCanceled());

    ASSERT_EQ(testObject.counter(), expected);
}

#include "test03_onProperty.moc"
