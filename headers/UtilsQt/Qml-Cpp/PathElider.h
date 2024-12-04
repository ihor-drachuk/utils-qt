/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QFont>
#include <tuple>
#include <optional>
#include <vector>
#include <utils-cpp/default_ctor_ops.h>
#include <utils-cpp/pimpl.h>

class QRegularExpression;

struct PathEliderDecomposition
{
public:
    QString protocol;
    QStringList subdirs;
    std::optional<QChar> separator;
    QString name;

    auto tie() const { return std::tie(protocol, subdirs, separator, name); }
    bool operator== (const PathEliderDecomposition& rhs) const { return tie() == rhs.tie(); }
    bool operator!= (const PathEliderDecomposition& rhs) const { return tie() != rhs.tie(); }
    QChar getSeparator() const { return separator ? *separator : QChar(); };
    QString getSeparatorStr() const { return separator ? QString(*separator) : ""; };
    QString combine(std::vector<bool> skipDirs = {}) const;

public:
    Q_GADGET
    Q_PROPERTY(QString protocol MEMBER protocol)
    Q_PROPERTY(QStringList subdirs MEMBER subdirs)
    Q_PROPERTY(QChar separator READ getSeparator)
    Q_PROPERTY(QString name MEMBER name)
};

class PathElider : public QObject
{
    Q_OBJECT
    NO_COPY_MOVE(PathElider);
public:
    Q_PROPERTY(QString sourceText READ sourceText WRITE setSourceText NOTIFY sourceTextChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(int widthLimit READ widthLimit WRITE setWidthLimit NOTIFY widthLimitChanged)
    Q_PROPERTY(QString elidedText READ elidedText /*WRITE setElidedText*/ NOTIFY elidedTextChanged)

    static void registerTypes();

    explicit PathElider(QObject* parent = nullptr);
    ~PathElider() override;

    Q_INVOKABLE PathEliderDecomposition decomposePath(const QString& path) const;

// --- Properties support ---
public:
    const QString& sourceText() const;
    void setSourceText(const QString& value);
    const QFont& font() const;
    void setFont(const QFont& value);
    int widthLimit() const;
    void setWidthLimit(int value);
    const QString& elidedText() const;

private:
    void setElidedText(const QString& value);

signals:
    void sourceTextChanged(const QString& sourceText);
    void fontChanged(const QFont& font);
    void widthLimitChanged(int widthLimit);
    void elidedTextChanged(const QString& elidedText);
// --- ---

private:
    static QStringList separateSubdirs(const QString& path);
    void recalculate();

private:
    DECLARE_PIMPL
};
