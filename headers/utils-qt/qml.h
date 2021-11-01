#pragma once

class QQmlEngine;
class QGuiApplication;

namespace UtilsQt {
namespace Qml {

void registerAll(QQmlEngine& engine, QGuiApplication& app);

} // namespace Qml
} // namespace UtilsQt
