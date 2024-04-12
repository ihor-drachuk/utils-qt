/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QVariant>

namespace UtilsQt::QVariantMigration {

enum TypeId {
    Invalid = QMetaType::UnknownType,
    Bool = QMetaType::Bool,
    Int = QMetaType::Int,
    UInt = QMetaType::UInt,
    LongLong = QMetaType::LongLong,
    ULongLong = QMetaType::ULongLong,
    Double = QMetaType::Double,
    Char = QMetaType::QChar,
    Map = QMetaType::QVariantMap,
    List = QMetaType::QVariantList,
    String = QMetaType::QString,
    StringList = QMetaType::QStringList,
    ByteArray = QMetaType::QByteArray,
    BitArray = QMetaType::QBitArray,
    Date = QMetaType::QDate,
    Time = QMetaType::QTime,
    DateTime = QMetaType::QDateTime,
    Url = QMetaType::QUrl,
    Locale = QMetaType::QLocale,
    Rect = QMetaType::QRect,
    RectF = QMetaType::QRectF,
    Size = QMetaType::QSize,
    SizeF = QMetaType::QSizeF,
    Line = QMetaType::QLine,
    LineF = QMetaType::QLineF,
    Point = QMetaType::QPoint,
    PointF = QMetaType::QPointF,
#if QT_CONFIG(regularexpression)
    RegularExpression = QMetaType::QRegularExpression,
#endif
    Hash = QMetaType::QVariantHash,
#if QT_CONFIG(easingcurve)
    EasingCurve = QMetaType::QEasingCurve,
#endif
    Uuid = QMetaType::QUuid,
#if QT_CONFIG(itemmodel)
    ModelIndex = QMetaType::QModelIndex,
    PersistentModelIndex = QMetaType::QPersistentModelIndex,
#endif
    LastCoreType = QMetaType::LastCoreType,

    Font = QMetaType::QFont,
    Pixmap = QMetaType::QPixmap,
    Brush = QMetaType::QBrush,
    Color = QMetaType::QColor,
    Palette = QMetaType::QPalette,
    Image = QMetaType::QImage,
    Polygon = QMetaType::QPolygon,
    Region = QMetaType::QRegion,
    Bitmap = QMetaType::QBitmap,
    Cursor = QMetaType::QCursor,
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#if QT_CONFIG(shortcut)
    KeySequence = QMetaType::QKeySequence,
#endif // QT_CONFIG(shortcut)
#else
    KeySequence = QMetaType::QKeySequence,
#endif // QT_VERSION >= 6.0.0
    Pen = QMetaType::QPen,
    TextLength = QMetaType::QTextLength,
    TextFormat = QMetaType::QTextFormat,
    Transform = QMetaType::QTransform,
    Matrix4x4 = QMetaType::QMatrix4x4,
    Vector2D = QMetaType::QVector2D,
    Vector3D = QMetaType::QVector3D,
    Vector4D = QMetaType::QVector4D,
    Quaternion = QMetaType::QQuaternion,
    PolygonF = QMetaType::QPolygonF,
    Icon = QMetaType::QIcon,
    LastGuiType = QMetaType::LastGuiType,

    SizePolicy = QMetaType::QSizePolicy,

    Long = QMetaType::Long,
    ULong = QMetaType::ULong,
    Short = QMetaType::Short,
    UShort = QMetaType::UShort,
    Float = QMetaType::Float,
    QReal = QMetaType::QReal,

    UserType = QMetaType::User,
    LastType = 0xffffffff // need this so that gcc >= 3.4 allocates 32 bits for Type
};

int getTypeId(const QVariant& value);

} // namespace UtilsQt::QVariantMigration
