#include <gtest/gtest.h>
#include <utils-qt/on_property.h>
#include <utils-qt/futureutils.h>
#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>

#ifdef UTILS_QT_CI_RUN
constexpr float timeFactor = 2;
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

public slots:
    void setCounter(int counter)
    {
        if (m_counter == counter)
            return;

        m_counter = counter;
        emit counterChanged(m_counter);
    }

signals:
    void counterChanged(const int& counter);

private:
    int m_counter { 0 };
    QTimer m_timer;
};


TEST(UtilsQt, onProperty_once)
{
    int triggeredCount = 0;
    QObject context;
    QElapsedTimer elapsedTimer;

    TestObject testObject;
    elapsedTimer.start();

    QEventLoop loop;

    onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 4, true, &context, [&]()
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

TEST(UtilsQt, onProperty_multiple)
{
    int triggeredCount = 0;
    QObject* context = new QObject;
    TestObject testObject(true);

    onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 1, false, context, [&]()
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

TEST(UtilsQt, onProperty_initial)
{
    int triggeredCount = 0;
    QObject context;
    TestObject testObject;

    onProperty(&testObject, &TestObject::counter, &TestObject::counterChanged, 0, true, &context, [&]()
    {
        triggeredCount++;
    });

    ASSERT_EQ(triggeredCount, 1);
}

#include "test4_onProperty.moc"
