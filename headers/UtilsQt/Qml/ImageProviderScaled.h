#pragma once
#include <QQuickImageProvider>

class ImageProviderScaled : public QQuickImageProvider
{
public:
    static void registerTypes(QQmlEngine& engine);

    ImageProviderScaled(): QQuickImageProvider(ImageType::Image) { }

public: // QQuickImageProvider interface
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};
