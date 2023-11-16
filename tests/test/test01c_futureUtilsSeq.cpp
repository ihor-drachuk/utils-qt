/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <string>
#include <QEventLoop>
#include <UtilsQt/Futures/futureutils_sequential.h>

using namespace UtilsQt;


TEST(UtilsQt, FutureUtilsSeqTest_basic)
{
    QObject ctx;
    float result = 0;
    bool errorFlag = false;

    auto done =
    connectFutureSeq(createTimedFuture(20, 170), &ctx)
         .then([](const std::optional<int>& value) -> QFuture<std::string> { return createReadyFuture(std::to_string(value.value() + 1)); })
         .then([](const std::optional<std::string>& value) -> QFuture<float> { return createTimedFuture(40, (float)std::stoi(value.value())); })
         .then([&result](const std::optional<float>& value) { result = value.value(); })
         .onError([&errorFlag](std::exception_ptr) { errorFlag = true; })
         .readyPromise();

    waitForFuture<QEventLoop>(done);

    ASSERT_FALSE(errorFlag);
    ASSERT_TRUE(qFuzzyCompare(result, 171));
}



TEST(UtilsQt, FutureUtilsSeqTest_cancel_getFuture)
{
    QObject ctx;
    bool errorFlag = false;

    bool visited[4] = {false};
    bool valueProvided[3] = {false};

    QFuture<int> result1;
    QFuture<std::string> result2;
    QFuture<float> result3;

    auto done =
    connectFutureSeq(createTimedFuture(20, 170), &ctx)
         .getFuture(result1)
         .then([&](const std::optional<int>& value) -> QFuture<std::string> { visited[0] = true; valueProvided[0] = value.has_value(); return createReadyFuture(std::to_string(value.value() + 1)); })
         .getFuture(result2)
         .then([&](const std::optional<std::string>& value) -> QFuture<float> { visited[1] = true; valueProvided[1] = value.has_value(); return createCanceledFuture<float>(); })
         .getFuture(result3)
         .then([&](const std::optional<float>& value) { visited[2] = true; valueProvided[2] = value.has_value(); })
         .onError([&](std::exception_ptr ex)
         {
            visited[3] = true;
            errorFlag = true;
            try {
                if (ex)
                    std::rethrow_exception(ex);
            } catch (const std::exception& e) {
                std::cerr << "Caught exception: " << e.what() << std::endl;
                std::cerr.flush();
            }
         })
         .readyPromise();

    waitForFuture<QEventLoop>(done);

    ASSERT_TRUE(result1.isFinished());
    ASSERT_TRUE(result2.isFinished());
    ASSERT_TRUE(result3.isFinished());
    ASSERT_FALSE(result1.isCanceled());
    ASSERT_FALSE(result2.isCanceled());
    ASSERT_TRUE(result3.isCanceled());
    ASSERT_EQ(result1.result(), 170);
    ASSERT_EQ(result2.result(), "171");

    ASSERT_FALSE(errorFlag);

    ASSERT_EQ(visited[0], true);
    ASSERT_EQ(visited[1], true);
    ASSERT_EQ(visited[2], true);
    ASSERT_EQ(visited[3], false);

    ASSERT_EQ(valueProvided[0], true);
    ASSERT_EQ(valueProvided[1], true);
    ASSERT_EQ(valueProvided[2], false);
}


TEST(UtilsQt, FutureUtilsSeqTest_exception)
{
    QObject ctx;
    float result = 0;
    bool errorFlag = false;

    auto done =
    connectFutureSeq(createTimedFuture(20, 170), &ctx)
         .then([](const std::optional<int>& value) -> QFuture<std::string> { return createReadyFuture(std::to_string(value.value() + 1)); })
         .then([](const std::optional<std::string>& /*value*/) -> QFuture<float> { throw 1; return {}; })
         .then([&result](const std::optional<float>& value) { result = value.value(); })
         .onError([&errorFlag](std::exception_ptr) { errorFlag = true; })
         .readyPromise();

    waitForFuture<QEventLoop>(done);

    ASSERT_TRUE(errorFlag);
    ASSERT_TRUE(qFuzzyCompare(result, 0));
}


TEST(UtilsQt, FutureUtilsSeqTest_context)
{
    auto ctx = new QObject();
    float result = 0;
    bool errorFlag = false;
    bool value3Called = false;

    auto done =
    connectFutureSeq(createTimedFuture(20, 170), ctx)
         .then([](const std::optional<int>& value) -> QFuture<std::string> { return createReadyFuture(std::to_string(value.value() + 1)); })
         .then([&ctx](const std::optional<std::string>& value) -> QFuture<float> { ctx->deleteLater(); return createTimedFuture(40, (float)std::stoi(value.value())); })
         .then([&](const std::optional<float>& value) { value3Called = true; result = value.value(); })
         .onError([&errorFlag](std::exception_ptr) { errorFlag = true; })
         .readyPromise();

    waitForFuture<QEventLoop>(done);

    ASSERT_FALSE(errorFlag);
    ASSERT_FALSE(value3Called);
    ASSERT_TRUE(qFuzzyCompare(result, 0));
}
