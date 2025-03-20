/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml/ImageProviderBorderImage.h>

#include <UtilsQt/Qml-Cpp/QmlUtils.h>
#include <QString>
#include <QImage>
#include <QQmlEngine>
#include <QPainter>

static QMap<QString, QString> parseParams(const QString &id)
{
    QMap<QString, QString> params;

    static const QSet<QString> allowedKeys = {"path", "top", "bottom", "left", "right", "width", "height", "orientation", "fill"};

    const QStringList list = id.split('&', Qt::SkipEmptyParts);
    for (const QString &pair : list) {
        int eq = pair.indexOf('=');
        if (eq > 0) {
            QString key = pair.left(eq).trimmed();
            QString value = pair.mid(eq + 1).trimmed();
            assert(allowedKeys.contains(key) && "Unexpected parameter");
            params.insert(key, value);
        }
    }
    return params;
}

void ImageProviderBorderImage::registerTypes(QQmlEngine& engine)
{
    engine.addImageProvider("UtilsQt-BorderImage", new ImageProviderBorderImage());
}

QImage ImageProviderBorderImage::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    assert(size);

    if (requestedSize.width() <= 0 || requestedSize.height() <= 0) {
        QImage stub(10, 10, QImage::Format_ARGB32);
        stub.fill(QColor::fromRgbF(0.5, 0.5, 0.5));
        *size = stub.size();
        return stub;
    }

    // Parse the id string for parameters.
    QMap<QString, QString> params = parseParams(id);

    // Mandatory parameters
    const QString path = params.value("path");
    const int finalWidth = requestedSize.width();
    const int finalHeight = requestedSize.height();
    const QString orientation = params.value("orientation", "vertical").toLower();
    const QString fillMode = params.value("fill", "stretch").toLower();

    // Optional border parameters (defaulting to 0 if missing)
    const int top = params.contains("top") ? params.value("top").toInt() : 0;
    const int bottom = params.contains("bottom") ? params.value("bottom").toInt() : 0;
    const int left = params.contains("left") ? params.value("left").toInt() : 0;
    const int right = params.contains("right") ? params.value("right").toInt() : 0;

    // Return final image size as requested.
    *size = QSize(finalWidth, finalHeight);

    // Load the source image (using a normalized path, if available).
    const QString normalizedPath = QmlUtils::instance().normalizePath(path);
    QImage source(normalizedPath);
    if (source.isNull()) {
        qWarning() << "Failed to load source image:" << normalizedPath;
        return QImage(finalWidth, finalHeight, QImage::Format_ARGB32);
    }

    const int origWidth = source.width();
    const int origHeight = source.height();

    // Compute the scale factor based on the orientation.
    double scaleFactor = 1.0;
    if (orientation == "vertical") {
        scaleFactor = double(finalWidth) / double(origWidth);
    } else if (orientation == "horizontal") {
        scaleFactor = double(finalHeight) / double(origHeight);
    } else {
        assert(false && "Unsupported orientation");
    }

    // Scale the entire source image proportionally.
    const int scaledWidth = qRound(origWidth * scaleFactor);
    const int scaledHeight = qRound(origHeight * scaleFactor);
    QImage scaledImage = source.scaled(scaledWidth, scaledHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Scale the border parameters accordingly.
    const int topScaled = qRound(top * scaleFactor);
    const int bottomScaled = qRound(bottom * scaleFactor);
    const int leftScaled = qRound(left * scaleFactor);
    const int rightScaled = qRound(right * scaleFactor);

    // Create the final image of exactly the requested size.
    QImage finalImage(finalWidth, finalHeight, QImage::Format_ARGB32);
    finalImage.fill(Qt::transparent);
    QPainter painter(&finalImage);

    // Dimensions of the scaled image.
    const int srcW = scaledImage.width();
    const int srcH = scaledImage.height();

    // --- Define source rectangles from the scaled image ---
    // Corners:
    QRect srcTopLeft(0, 0, leftScaled, topScaled);
    QRect srcTopRight(srcW - rightScaled, 0, rightScaled, topScaled);
    QRect srcBottomLeft(0, srcH - bottomScaled, leftScaled, bottomScaled);
    QRect srcBottomRight(srcW - rightScaled, srcH - bottomScaled, rightScaled, bottomScaled);
    // Edges:
    QRect srcTopEdge(leftScaled, 0, srcW - leftScaled - rightScaled, topScaled);
    QRect srcBottomEdge(leftScaled, srcH - bottomScaled, srcW - leftScaled - rightScaled, bottomScaled);
    QRect srcLeftEdge(0, topScaled, leftScaled, srcH - topScaled - bottomScaled);
    QRect srcRightEdge(srcW - rightScaled, topScaled, rightScaled, srcH - topScaled - bottomScaled);
    // Center:
    QRect srcCenter(leftScaled, topScaled, srcW - leftScaled - rightScaled, srcH - topScaled - bottomScaled);

    // --- Define target rectangles in the final image ---
    // Corners: (drawn at their native scaled sizes)
    QRect tgtTopLeft(0, 0, leftScaled, topScaled);
    QRect tgtTopRight(finalWidth - rightScaled, 0, rightScaled, topScaled);
    QRect tgtBottomLeft(0, finalHeight - bottomScaled, leftScaled, bottomScaled);
    QRect tgtBottomRight(finalWidth - rightScaled, finalHeight - bottomScaled, rightScaled, bottomScaled);
    // Edges:
    QRect tgtTopEdge(leftScaled, 0, finalWidth - leftScaled - rightScaled, topScaled);
    QRect tgtBottomEdge(leftScaled, finalHeight - bottomScaled, finalWidth - leftScaled - rightScaled, bottomScaled);
    QRect tgtLeftEdge(0, topScaled, leftScaled, finalHeight - topScaled - bottomScaled);
    QRect tgtRightEdge(finalWidth - rightScaled, topScaled, rightScaled, finalHeight - topScaled - bottomScaled);
    // Center:
    QRect tgtCenter(leftScaled, topScaled, finalWidth - leftScaled - rightScaled, finalHeight - topScaled - bottomScaled);

    // Lambda for drawing a region with the chosen fill mode (used for edges and center).
    auto drawRegion = [&](const QRect& target, const QRect& src) {
        if (!src.isEmpty() && !target.isEmpty()) {
            if (fillMode == "tile") {
                QPixmap tilePixmap = QPixmap::fromImage(scaledImage.copy(src));
                painter.drawTiledPixmap(target, tilePixmap);
            } else if (fillMode == "stretch") {
                painter.drawImage(target, scaledImage, src);
            } else {
                assert(false && "Unsupported fill mode");
            }
        }
    };

    // --- Draw each region with proper scaling ---
    // Corners: drawn as-is.
    if (!srcTopLeft.isEmpty() && !tgtTopLeft.isEmpty())
        painter.drawImage(tgtTopLeft, scaledImage, srcTopLeft);
    if (!srcTopRight.isEmpty() && !tgtTopRight.isEmpty())
        painter.drawImage(tgtTopRight, scaledImage, srcTopRight);
    if (!srcBottomLeft.isEmpty() && !tgtBottomLeft.isEmpty())
        painter.drawImage(tgtBottomLeft, scaledImage, srcBottomLeft);
    if (!srcBottomRight.isEmpty() && !tgtBottomRight.isEmpty())
        painter.drawImage(tgtBottomRight, scaledImage, srcBottomRight);

    // Edges: scaled in one dimension.
    drawRegion(tgtTopEdge, srcTopEdge);
    drawRegion(tgtBottomEdge, srcBottomEdge);
    drawRegion(tgtLeftEdge, srcLeftEdge);
    drawRegion(tgtRightEdge, srcRightEdge);

    // Center: scaled in both dimensions.
    drawRegion(tgtCenter, srcCenter);

    painter.end();

    return finalImage;
}
