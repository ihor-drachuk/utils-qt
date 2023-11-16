/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml/ImageProviderScaled.h>

#include <UtilsQt/Qml-Cpp/QmlUtils.h>
#include <QString>
#include <QImage>
#include <QQmlEngine>

void ImageProviderScaled::registerTypes(QQmlEngine& engine)
{
    engine.addImageProvider("UtilsQt-Scaled", new ImageProviderScaled());
}

QImage ImageProviderScaled::requestImage(const QString& id,
                                           QSize* size,
                                           const QSize& requestedSize)
{
    // TODO: Log id, requestedSize, errors inside
    assert(size);
    assert(requestedSize.isValid());

    const auto sourceSize = QmlUtils::instance().imageSize(id);
    assert(sourceSize.isValid() && !sourceSize.isNull());
    *size = sourceSize;

    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        const auto path = QmlUtils::instance().normalizePath(id);
        const QImage image(path);
        assert(!image.isNull());

        return image.scaled(requestedSize,
                            Qt::AspectRatioMode::IgnoreAspectRatio,
                            Qt::TransformationMode::SmoothTransformation);
    } else {
        return QImage(1, 1, QImage::Format::Format_ARGB32);
    }
}
