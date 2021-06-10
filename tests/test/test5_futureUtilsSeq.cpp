#include <gtest/gtest.h>

#include <string>
#include <QEventLoop>
#include <utils-qt/futureutils_sequential.h>


TEST(UtilsQt, FutureUtilsSeqTest_basic)
{
    QObject ctx;
    float result = 0;
    bool errorFlag = false;

    auto done =
    connectFutureSeq(createTimedFuture2(20, 170), &ctx)
         .then([](const std::optional<int>& value) -> QFuture<std::string> { return createReadyFuture(std::to_string(value.value() + 1)); })
         .then([](const std::optional<std::string>& value) -> QFuture<float> { return createTimedFuture2(40, (float)std::stoi(value.value())); })
         .then([&result](const std::optional<float>& value) { result = value.value(); })
         .onError([&errorFlag](std::exception_ptr) { errorFlag = true; })
         .readyPromise();

    waitForFuture<QEventLoop>(done);

    ASSERT_FALSE(errorFlag);
    ASSERT_TRUE(qFuzzyCompare(result, 171));
}


TEST(UtilsQt, FutureUtilsSeqTest_exception)
{
    QObject ctx;
    float result = 0;
    bool errorFlag = false;

    auto done =
    connectFutureSeq(createTimedFuture2(20, 170), &ctx)
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
    connectFutureSeq(createTimedFuture2(20, 170), ctx)
         .then([](const std::optional<int>& value) -> QFuture<std::string> { return createReadyFuture(std::to_string(value.value() + 1)); })
         .then([&ctx](const std::optional<std::string>& value) -> QFuture<float> { ctx->deleteLater(); return createTimedFuture2(40, (float)std::stoi(value.value())); })
         .then([&](const std::optional<float>& value) { value3Called = true; result = value.value(); })
         .onError([&errorFlag](std::exception_ptr) { errorFlag = true; })
         .readyPromise();

    waitForFuture<QEventLoop>(done);

    ASSERT_FALSE(errorFlag);
    ASSERT_FALSE(value3Called);
    ASSERT_TRUE(qFuzzyCompare(result, 0));
}
