/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/MaskInverter.h>

#include <QQmlEngine>

void MaskInverter::registerTypes()
{
    qmlRegisterType<MaskInverter>("UtilsQt", 1, 0, "MaskInverter");
}

MaskInverter::MaskInverter(QQuickItem* parent)
    : QQuickItem(parent)
{
}

bool MaskInverter::contains(const QPointF& point) const
{
    const auto childItems = this->childItems();
    return std::none_of(childItems.begin(), childItems.end(), [point, this](QQuickItem* child) {
        return child->contains(mapToItem(child, point));
    });
}
