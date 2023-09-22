#include <gtest/gtest.h>
#include <UtilsQt/OnProperty.h>
#include <UtilsQt/Futures/Utils.h>
#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>

using namespace UtilsQt;

#ifdef UTILS_QT_OS_MACOS
constexpr float timeFactor = 3;
#else
constexpr float timeFactor = 1;
#endif

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
        ASSERT_GE(elapsedTimer.elapsed(), 350 / timeFactor);
        ASSERT_LE(elapsedTimer.elapsed(), 450 * timeFactor);

        waitForFuture<QEventLoop>(createTimedFuture(400));
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
        ASSERT_GE(elapsedTimer.elapsed(), 350 / timeFactor);
        ASSERT_LE(elapsedTimer.elapsed(), 450 * timeFactor);

        waitForFuture<QEventLoop>(createTimedFuture(400));
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

    waitForFuture<QEventLoop>(createTimedFuture(800));
    ASSERT_GE(triggeredCount, 3 / timeFactor);
    ASSERT_LE(triggeredCount, 5 * timeFactor);

    auto savedCount = triggeredCount;
    delete context; context = nullptr;
    waitForFuture<QEventLoop>(createTimedFuture(800));
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

            ASSERT_GE(elapsed, 100 / timeFactor);
            ASSERT_LE(elapsed, 200 * timeFactor);
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
