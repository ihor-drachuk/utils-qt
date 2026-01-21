<p align="center">
  <h1 align="center">utils-qt</h1>
  <p align="center">
    <strong>Comprehensive Utility Library for Qt Applications</strong>
  </p>
  <p align="center">
    Modern C++17 utilities for async operations, QML integration, and model manipulation
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

- [Requirements](#requirements)
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

## Requirements

| Requirement | Version |
|-------------|---------|
| C++ Standard | C++17 |
| CMake | 3.16+ |
| Qt | 5.15+ or 6.x |
| Compiler | GCC 9+, Clang 10+, or MSVC 2019+ |

### Supported Platforms

- **Windows** (MSVC 2019+)
- **Linux** (GCC 9+, Clang 10+)
- **macOS** (Apple Clang, M1/M2 supported)

---

## Installation

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
#include <UtilsQt/Qml/qml.h>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    UtilsQt::Qml::registerAll(&engine);

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
| `UTILS_QT_GTEST_SEARCH_MODE` | Auto | GTest search: Auto, Force, Skip |

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
    [](const QString& s) { return s.toInt(); }
);
```

**Automatic retry logic:**

```cpp
#include <UtilsQt/Futures/RetryingFuture.h>

auto future = UtilsQt::createRetryingFuture(
    context,
    []() { return fetchDataAsync(); },  // Async operation
    [](const Data& d) {                  // Validator
        return d.isValid() ? ValidationDecision::ResultIsValid :
                             ValidationDecision::NeedRetry;
    },
    5,    // Max retries
    1000  // Retry interval (ms)
);
```

**Convert Qt signals to futures:**

```cpp
#include <UtilsQt/Futures/SignalToFuture.h>

auto future = UtilsQt::signalToFuture(&object, &MyClass::dataReady);
// Returns QFuture with signal argument type
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
#include <UtilsQt/Qml-Cpp/AugmentedModel.h>

AugmentedModel model;
model.setSourceModel(sourceModel);
model.addCalculatedRole("fullName", {"firstName", "lastName"},
    [](const QVariantMap& data) {
        return data["firstName"].toString() + " " + data["lastName"].toString();
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
    UtilsQt::OnProperty::Comparison::Equal,
    true,                                    // Once
    context,
    []() { qDebug() << "Ready!"; },
    5000,                                    // Timeout (ms)
    [](auto reason) { qDebug() << "Canceled"; }
);

// Or get a future instead
auto future = UtilsQt::onPropertyFuture(
    object, &MyClass::status, &MyClass::statusChanged,
    Status::Ready
);
```

---

### Data Conversion

**Type-safe QVariant conversion:**

```cpp
#include <UtilsQt/qvariant_conv.h>

QVariant var = getVariant();

// Safe loading with checks
int value;
if (UtilsQt::load(var, value, UtilsQt::Check_Type)) {
    // value is valid
}

// Or with optional
auto opt = UtilsQt::load<double>(var, UtilsQt::Check_ConvResult);
if (opt) {
    double d = *opt;
}
```

**Enum utilities:**

```cpp
#include <UtilsQt/enum_utils.h>

enum class Color { Red, Green, Blue };
Q_ENUM(Color)

auto str = UtilsQt::toString(Color::Red);        // "Red"
auto val = UtilsQt::fromString<Color>("Green");  // Color::Green
auto names = UtilsQt::allNames<Color>();         // {"Red", "Green", "Blue"}
bool valid = UtilsQt::isValid<Color>(value);
```

---

### JSON Validation

Comprehensive JSON structure validation:

```cpp
#include <UtilsQt/JsonValidator.h>

using namespace UtilsQt::JsonValidator;

Logger logger;
bool valid = check(jsonValue, logger,
    Object {
        {"name", String{}},
        {"age", Integer{}},
        {"email", Optional{String{}}},
        {"tags", Array{String{}}}
    }
);

if (!valid) {
    for (const auto& error : logger.errors()) {
        qWarning() << error;
    }
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
| `Futures/Utils.h` | Core utilities: `onFinished`, `onResult`, `onCanceled`, `Promise`, `createReadyFuture`, `createTimedFuture` |
| `Futures/Merge.h` | Combine futures: `mergeFuturesAll`, `mergeFuturesAny` |
| `Futures/Converter.h` | Transform futures: `convertFuture` |
| `Futures/RetryingFuture.h` | Auto-retry: `createRetryingFuture` |
| `Futures/SignalToFuture.h` | Signal conversion: `signalToFuture` |

### QML-Cpp Module

| Component | Description |
|-----------|-------------|
| `QmlUtils` | Singleton with clipboard, path, image, color utilities |
| `FileWatcher` | Monitor file changes |
| `PathElider` | Elide long paths for display |
| `AugmentedModel` | Add calculated roles to models |
| `MergedListModel` | Join two models by key |
| `PlusOneProxyModel` | Add artificial rows |
| `ListModelItemProxy` | Track single item |
| `Multibinding` | Synchronize multiple properties |
| `NumericalValidator` | Numeric input validation |
| `Geometry` | Polygon operations |

### Core Utilities

| Header | Description |
|--------|-------------|
| `qvariant_conv.h` | Type-safe QVariant conversion |
| `enum_utils.h` | Enum serialization utilities |
| `JsonValidator.h` | JSON structure validation |
| `OnProperty.h` | Property change monitoring |
| `Multicontext.h` | Shared lifetime management |
| `dpitools.h` | DPI/scaling configuration |

---

## License

MIT License — see [License.txt](License.txt) for details.

Copyright (c) 2018-2026 Ihor Drachuk

---

## Author

**Ihor Drachuk** — [ihor-drachuk-libs@pm.me](mailto:ihor-drachuk-libs@pm.me)

[GitHub](https://github.com/ihor-drachuk/utils-qt)

