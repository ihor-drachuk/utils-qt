<p align="center">
  <h1 align="center">utils-qt</h1>
  <p align="center">
    <strong>Extended Qt toolkit</strong>
  </p>
  <p align="center">
    Qt utilities for async operations, QML integration, and model manipulation
  </p>
</p>

<p align="center">
  <a href="https://github.com/ihor-drachuk/utils-qt/actions/workflows/ci.yml"><img src="https://github.com/ihor-drachuk/utils-qt/actions/workflows/ci.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/ihor-drachuk/utils-qt/blob/master/License.txt"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17">
  <img src="https://img.shields.io/badge/Qt-5.15%20|%206.x-green.svg" alt="Qt">
  <img src="https://img.shields.io/badge/Platform-Windows%20|%20Linux%20|%20macOS-blueviolet.svg" alt="Platform">
</p>

---

## Table of Contents

- [Installation](#installation)
- [Build Options](#build-options)
- [Key Features](#key-features)
  - [Futures & Asynchronous Operations](#futures--asynchronous-operations)
  - [QML Utilities](#qml-utilities)
  - [Model Utilities](#model-utilities)
  - [Multibinding System](#multibinding-system)
  - [Property Watching](#property-watching)
  - [Data Conversion](#data-conversion)
  - [JSON Validation](#json-validation)
  - [Geometry Operations](#geometry-operations)
  - [Validators](#validators)
- [API Reference](#api-reference)
- [License](#license)
- [Contact](#contact)

---

## Installation

The library depends on [utils-cpp](https://github.com/ihor-drachuk/utils-cpp), which is bundled as a submodule and built automatically.

### Option 1: FetchContent (Recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
    utils-qt
    GIT_REPOSITORY https://github.com/ihor-drachuk/utils-qt.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(utils-qt)

target_link_libraries(your_target PRIVATE utils-qt)
```

### Option 2: Subdirectory

```cmake
add_subdirectory(path/to/utils-qt)
target_link_libraries(your_target PRIVATE utils-qt)
```

### QML Registration

```cpp
#include <UtilsQt/qml.h>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    UtilsQt::Qml::registerAll(engine);

    // ...
}
```

---

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `UTILS_QT_ENABLE_TESTS` | OFF | Enable unit tests |
| `UTILS_QT_ENABLE_BENCHMARK` | OFF | Enable benchmarks |
| `UTILS_QT_NO_GUI_TESTS` | OFF | Disable GUI-dependent tests |
| `UTILS_QT_WERROR` | OFF | Treat warnings as errors |
| `UTILS_QT_GTEST_SEARCH_MODE` | Auto | GTest search: Auto, Force, Skip |
| `UTILS_QT_LONG_INTERVALS` | OFF | Enable long intervals in tests |

---

## Key Features

### Futures & Asynchronous Operations

**Promise-based async programming** with full cancellation support and lifetime management.

```cpp
#include <UtilsQt/Futures/Utils.h>

// Create a promise for manual control
auto promise = UtilsQt::createPromise<QString>(true);
promise.finish("Result");

// Handle future completion
UtilsQt::onFinished(future, context, [](std::optional<QString> result) {
    if (result) {
        qDebug() << "Success:" << *result;
    } else {
        qDebug() << "Canceled";
    }
});

// Create ready/canceled/timed futures
auto ready = UtilsQt::createReadyFuture<int>(42);
auto canceled = UtilsQt::createCanceledFuture<int>();
auto timed = UtilsQt::createTimedFuture<QString>(1000, "Delayed result");
```

**Sequential async chains** with cancellation support and mediator:

```cpp
#include <UtilsQt/Futures/Sequential.h>

auto future = UtilsQt::Sequential(context)
    .start([]() {
        return fetchUserAsync();
    })
    .then([](const UtilsQt::AsyncResult<User>& result) {
        return fetchDetailsAsync(result.value().id);
    })
    .then([](const UtilsQt::AsyncResult<Details>& result) {
        return processAsync(result.value());
    })
    .execute();
```

**Merge multiple futures** with configurable cancellation behavior:

```cpp
#include <UtilsQt/Futures/Merge.h>

auto f1 = createTimedFuture<std::string>(100, "str");
auto f2 = createTimedFuture<double>(110, 123.83);

// Wait for all futures
auto all = UtilsQt::mergeFuturesAll(this, f1, f2);
// Result type: QFuture<std::tuple<std::optional<std::string>, std::optional<double>>>

// React to first completion
auto any = UtilsQt::mergeFuturesAny(this, f1, f2);

// With flags
auto merged = UtilsQt::mergeFuturesAll(this,
    UtilsQt::MergeFlags::IgnoreSomeCancellation, f1, f2);
```

**Convert and chain futures:**

```cpp
#include <UtilsQt/Futures/Converter.h>

// Transform future types
auto intFuture = UtilsQt::convertFuture<QString, int>(
    context, stringFuture,
    [](const QString& s) -> std::optional<int> { return s.toInt(); }
);
```

**Automatic retry logic:**

```cpp
#include <UtilsQt/Futures/RetryingFuture.h>

auto future = UtilsQt::createRetryingFuture(
    context,
    []() { return fetchDataAsync(); },    // Async operation
    [](const std::optional<Data>& result) // Validator (nullopt if canceled)
        -> UtilsQt::ValidatorDecision {
        return result ? ValidatorDecision::ResultIsValid :
                        ValidatorDecision::NeedRetry;
    },
    5,    // Max retries (or UtilsQt::RetryingFuture::UnlimitedCalls)
    1000  // Retry interval (ms)
);
```

**Convert Qt signals to futures:**

```cpp
#include <UtilsQt/Futures/SignalToFuture.h>

auto future = UtilsQt::signalToFuture(&object, &MyClass::dataReady);

// With timeout
auto future = UtilsQt::signalToFuture(&object, &MyClass::dataReady, context, 5000);
```

**Future broker** for transparent source replacement:

```cpp
#include <UtilsQt/Futures/Broker.h>

UtilsQt::Broker<QString> broker;
auto stableFuture = broker.future(); // Always valid, survives rebinds

broker.rebind(fetchAsync());   // Bind to first source
broker.rebind(fetchAsync());   // Transparently switch to new source
```

---

### QML Utilities

**QmlUtils singleton** provides comprehensive utilities accessible from QML:

```qml
import UtilsQt 1.0

Item {
    Component.onCompleted: {
        // Clipboard
        QmlUtils.clipboardSetText("Hello!")
        var text = QmlUtils.clipboardGetText()

        // Image operations
        var size = QmlUtils.imageSize("image.png")
        var isImg = QmlUtils.isImage("file.jpg")

        // Size with aspect ratio
        var fitted = QmlUtils.fitSize(sourceSize, limits)

        // Color manipulation
        var accent = QmlUtils.colorMakeAccent(baseColor, 0.2)
        var transparent = QmlUtils.colorChangeAlpha(color, 0.5)

        // Number utilities
        var bounded = QmlUtils.bound(0, value, 100)
        var equal = QmlUtils.doublesEqual(a, b, 0.001)

        // Regex extraction
        var match = QmlUtils.extractByRegex(text, "\\d+")

        // Size formatting
        var formatted = QmlUtils.sizeConv(1234567) // "1.2 MB"

        // System info
        var id = QmlUtils.machineUniqueId()
        var qtVer = QmlUtils.qtVersion

        // File operations
        var files = QmlUtils.listFiles(path, ["*.txt"], true)

        // Window state
        var visible = QmlUtils.isWindowVisible(Window.window)
        var foreground = QmlUtils.isWindowForeground(Window.window)

        // Delayed calls
        QmlUtils.callDelayed(this, function() { console.log("delayed") }, 100)
    }
}
```

**FileWatcher** for monitoring file changes:

```qml
FileWatcher {
    fileName: "/path/to/file.txt"
    onFileChanged: console.log("File modified!")

    // Available properties
    // fileExists, hasAccess, size, localFileName
}
```

---

### Model Utilities

**AugmentedModel** - Add calculated roles to existing models:

```cpp
#include <UtilsQt/AugmentedModel.h>

AugmentedModel model;
model.setSourceModel(sourceModel);
model.addCalculatedRole("fullName", {"firstName", "lastName"},
    [](const QVariantList& data) {
        return data[0].toString() + " " + data[1].toString();
    });
```

**MergedListModel** - Join two models by key:

```qml
MergedListModel {
    model1: usersModel
    model2: userDetailsModel
    joinRole1: "userId"
    joinRole2: "id"
}
```

**PlusOneProxyModel** - Add artificial rows:

```qml
PlusOneProxyModel {
    sourceModel: myModel
    mode: PlusOneProxyModel.Append  // or Prepend

    // Adds `isArtificial` and `artificialValue` roles
}
```

**ListModelItemProxy** - Track single item changes:

```qml
ListModelItemProxy {
    model: myListModel
    index: currentIndex

    onChanged: console.log("Item data changed")
    onRemoved: console.log("Item was removed")
}
```

---

### Multibinding System

Advanced property synchronization with loopback protection:

```qml
import UtilsQt 1.0

Multibinding {
    running: true

    MultibindingItem {
        object: slider
        propertyName: "value"
    }

    MultibindingItem {
        object: spinBox
        propertyName: "value"
        transformer: ScaleNum { factor: 100 }  // 0-1 to 0-100
    }

    MultibindingItem {
        object: textField
        propertyName: "text"
        transformer: JsTransformer {
            onReadConverter: function(v) { return v.toString() }
            onWriteConverter: function(v) { return parseFloat(v) }
        }
    }
}
```

---

### Property Watching

Monitor property changes with timeout support:

```cpp
#include <UtilsQt/OnProperty.h>

// Wait for property to reach expected value
UtilsQt::onProperty(
    object,
    &MyClass::status,
    &MyClass::statusChanged,
    Status::Ready,                           // Expected value
    UtilsQt::Comparison::Equal,
    true,                                    // Once
    context,
    []() { qDebug() << "Ready!"; },
    5000,                                    // Timeout (ms)
    [](auto reason) { qDebug() << "Canceled"; }
);

// Or get a future instead
auto future = UtilsQt::onPropertyFuture(
    object, &MyClass::status, &MyClass::statusChanged,
    Status::Ready, UtilsQt::Comparison::Equal, context
);
```

---

### Data Conversion

**Enum utilities:**

```cpp
#include <UtilsQt/enum_utils.h>

enum class Color { Red, Green, Blue };
Q_ENUM(Color)

auto str = Enum::toString(Color::Red);        // "Red"
auto val = Enum::fromString<Color>("Green");  // Color::Green
auto names = Enum::allNames<Color>();         // {"Red", "Green", "Blue"}
bool valid = Enum::isValid<Color>(value);
```

---

### JSON Validation

Comprehensive JSON structure validation:

```cpp
#include <UtilsQt/JsonValidator.h>

using namespace UtilsQt::JsonValidator;

auto validator = RootValidator(
    Object(
        Field("name", String()),
        Field("age", Integer()),
        Field("email", Optional, String()),
        Field("tags", Array(String()))
    )
);

ErrorInfo errorInfo;
bool valid = validator->check(errorInfo, jsonValue);

if (errorInfo.hasError()) {
    qWarning() << errorInfo.toString();
}
```

---

### Geometry Operations

Polygon manipulation from QML:

```qml
Geometry.polygonScale(polygon, 2.0, 2.0)
Geometry.isPolygonRectangular(polygon)
```

---

### Validators

**NumericalValidator** for input validation:

```qml
TextField {
    validator: NumericalValidator {
        bottom: 0
        top: 100
        step: 5
    }
}
```

---

## API Reference

### Futures Module

| Header | Description |
|--------|-------------|
| `Futures/Utils.h` | Core: `onFinished`, `onResult`, `onCanceled`, `onFinishedNoParamNoExcept`, `onCancelNotified`, `Promise`, `createReadyFuture`, `createTimedFuture`, `createExceptionFuture`, `getFutureState`, `hasResult` |
| `Futures/Sequential.h` | Sequential async chains: `Sequential`, `AsyncResult`, `Awaitables`, `SequentialMediator` |
| `Futures/Broker.h` | Future proxy: `Broker<T>` for transparent QFuture replacement |
| `Futures/Merge.h` | Combine futures: `mergeFuturesAll`, `mergeFuturesAny` |
| `Futures/Converter.h` | Transform futures: `convertFuture` |
| `Futures/RetryingFuture.h` | Auto-retry: `createRetryingFuture`, `createRetryingFutureRR` |
| `Futures/SignalToFuture.h` | Signal-to-future conversion with optional timeout |
| `Futures/Traits.h` | Type traits: `IsQFuture`, `QFutureUnwrap` |

### QML-Cpp Module

| Component | Description |
|-----------|-------------|
| `QmlUtils` | Singleton: clipboard, path, image, color, system info, delayed calls |
| `FileWatcher` | Monitor file changes |
| `PathElider` | Elide long paths for display |
| `AugmentedModel` | Add calculated roles to models |
| `MergedListModel` | Join two models by key |
| `PlusOneProxyModel` | Add artificial rows |
| `ListModelItemProxy` | Track single item |
| `ListModelTools` | Read model data, bulk collection via `collectData` / `collectDataByRoles` |
| `Multibinding` | Synchronize multiple properties |
| `NumericalValidator` | Numeric input validation |
| `Geometry` | Polygon operations |
| `SteadyTimer` | Monotonic timer (immune to system clock changes) |
| `FilterBehavior` | QML property interceptor with delay and conditional filtering |
| `PropertyInterceptor` | Property interceptor with before/after update signals |
| `ValueSequenceWatcher` | Detect specific value change sequences with timing |
| `MaskInverter` | Invert hit-test area of child items |
| `MaskInvertedRoundedRect` | Inverted rounded-rectangle hit-test mask |

### QML Components

| Component | Description |
|-----------|-------------|
| `DisabledVis` | Disabled state visual overlay |
| `GrainEffect` | Film grain shader effect |
| `Assert` | QML assertion helper |
| `Rectangle2` | Enhanced rectangle |
| `Repeater2` | Enhanced repeater |

### Image Providers

| Provider | Description |
|----------|-------------|
| `ImageProviderScaled` | Scaled image provider with caching |
| `ImageProviderBorderImage` | Border image provider with stretch/tile fill modes |

### Core Utilities

| Header | Description |
|--------|-------------|
| `qvariant_conv.h` | Type-safe QVariant conversion |
| `enum_utils.h` | Enum serialization utilities |
| `JsonValidator.h` | JSON structure validation (`ErrorInfo`, `LoggedErrorInfo`) |
| `OnProperty.h` | Property change monitoring |
| `Multicontext.h` | Shared lifetime management |
| `dpitools.h` | DPI/scaling configuration |
| `SetterWithDeferredSignal.h` | Set values first, emit signals at scope exit |
| `StdinListener.h` | Qt-wrapped stdin listener |
| `invoke_method.h` | Cross-Qt-version `invokeMethod` wrapper |
| `convertcontainer.h` | Qt container conversion helpers |
| `qvariant_traits.h` | QVariant type inspection utilities |
| `qvariant_migration.h` | Qt5/Qt6 QVariant compatibility layer |

---

## License

MIT License — see [License.txt](License.txt) for details.

Copyright (c) 2018-2026 Ihor Drachuk

---

## Author

**Ihor Drachuk** — [ihor-drachuk-libs@pm.me](mailto:ihor-drachuk-libs@pm.me)

[GitHub](https://github.com/ihor-drachuk/utils-qt)
