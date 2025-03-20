/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QQuickImageProvider>

// Description:
// This class provides a custom image provider for rendering border images.
// The difference between this provider and the default one is that this one
// supports image resizing before applying the border. This is useful when your
// asset's size doesn't match the desired final size of the image.
//
// To use this image provider, you should set the following in QML:
// Image {
//     //anchors.fill: parent  (optional)
//
//     source: "image://UtilsQt-BorderImage/path=...&orientation=...&top=...&bottom=...&left=...&right=...&fill=..."
//     sourceSize: Qt.size(width, height)
// }
//
// Where:
//  - 'path' is the path to the image asset. E.g.: "path=:/path/to/qrc-image.png"
//  - 'orientation' is the orientation of the image. E.g.: "horizontal" or "vertical"
//  - 'top', 'bottom', 'left', and 'right' are the border sizes in pixels (optional, default to 0)
//  - 'fill' is the fill mode. E.g.: "stretch" or "tile" (optional, default to "stretch")

class ImageProviderBorderImage : public QQuickImageProvider
{
public:
    static void registerTypes(QQmlEngine& engine);

    ImageProviderBorderImage(): QQuickImageProvider(ImageType::Image) { }
    ImageProviderBorderImage(const ImageProviderBorderImage&) = delete;

    ImageProviderBorderImage& operator=(const ImageProviderBorderImage&) = delete;

public: // QQuickImageProvider interface
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};
