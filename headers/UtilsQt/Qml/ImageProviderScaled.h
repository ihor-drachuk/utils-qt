/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QQuickImageProvider>

// To use this image provider, you should set the following in QML:
// Image {
//     //anchors.fill: parent  (optional)
//
//     source: "image://UtilsQt-Scaled/:/path/to/qrc-image.png"
//     sourceSize: Qt.size(width, height)
// }

class ImageProviderScaled : public QQuickImageProvider
{
public:
    static void registerTypes(QQmlEngine& engine);

    ImageProviderScaled(): QQuickImageProvider(ImageType::Image) { }
    ImageProviderScaled(const ImageProviderScaled&) = delete;

    ImageProviderScaled& operator=(const ImageProviderScaled&) = delete;

public: // QQuickImageProvider interface
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};
