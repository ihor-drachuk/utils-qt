/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/PathElider.h>

#include <QStringList>
#include <QQmlEngine>
#include <QFontMetrics>
#include <QRegularExpression>
#include <utils-cpp/middle_iterator.h>

struct PathElider::impl_t
{
    QString sourceText;
    QFont font;
    int widthLimit {};
    QString elidedText;

    QRegularExpression pathSeparatorRegexp;

    std::unique_ptr<QFontMetrics> fontMetrics;
};

void PathElider::registerTypes()
{
    qmlRegisterType<PathElider>("UtilsQt", 1, 0, "PathElider");
}

PathElider::PathElider(QObject* parent)
    : QObject(parent)
{
    createImpl();
    impl().pathSeparatorRegexp.setPattern(
        "^"
        "(:/|\\w+:/|/+|\\w:\\\\+)?"
        "(((.*?)([/\\\\]))*)"
        "(.*$)"
    );
}

PathElider::~PathElider()
{
}

PathEliderDecomposition PathElider::decomposePath(const QString& path) const
{
    constexpr int ProtocolsGroupIdx = 1;
    constexpr int DirsGroupIdx = 2;
    constexpr int SeparatorGroupIdx = 5;
    constexpr int NameGroupIdx = 6;

    auto match = impl().pathSeparatorRegexp.match(path);

    auto optSeparator = [](const QString& separator) -> std::optional<QChar> {
        if (separator.isEmpty()) {
            return {};
        } else {
            assert(separator.size() == 1);
            return separator[0];
        }
    };

    PathEliderDecomposition result;
    result.protocol = match.captured(ProtocolsGroupIdx);
    result.subdirs = separateSubdirs(match.captured(DirsGroupIdx));
    result.separator = optSeparator(match.captured(SeparatorGroupIdx));
    result.name = match.captured(NameGroupIdx);
    return result;
}

const QString& PathElider::sourceText() const
{
    return impl().sourceText;
}

void PathElider::setSourceText(const QString& value)
{
    if (impl().sourceText == value)
        return;
    impl().sourceText = value;
    emit sourceTextChanged(impl().sourceText);

    recalculate();
}

const QFont& PathElider::font() const
{
    return impl().font;
}

void PathElider::setFont(const QFont& value)
{
    if (impl().font == value)
        return;
    impl().font = value;
    emit fontChanged(impl().font);

    impl().fontMetrics = std::make_unique<QFontMetrics>(impl().font);
    recalculate();
}

int PathElider::widthLimit() const
{
    return impl().widthLimit;
}

void PathElider::setWidthLimit(int value)
{
    if (impl().widthLimit == value)
        return;
    impl().widthLimit = value;
    emit widthLimitChanged(impl().widthLimit);

    recalculate();
}

const QString& PathElider::elidedText() const
{
    return impl().elidedText;
}

void PathElider::setElidedText(const QString& value)
{
    if (impl().elidedText == value)
        return;
    impl().elidedText = value;
    emit elidedTextChanged(impl().elidedText);

    recalculate();
}

QStringList PathElider::separateSubdirs(const QString& path)
{
    QStringList result;
    int i{}, k{};

    auto isSeparator = [end = path.cend()](QString::const_iterator it){
        return (it == end || *it == '/' || *it == '\\');
    };

    for (auto it = path.cbegin(); ; it++, i++) {
        if (isSeparator(it)) {
            result.append(path.mid(k, i-k));
            k = i+1;
        }

        if (it == path.cend())
            break;
    }

    if (path.isEmpty() || isSeparator(path.cbegin() + path.size() - 1))
        result.removeLast();

    return result;
}

void PathElider::recalculate()
{
    if (!impl().fontMetrics ||
        impl().widthLimit == 0)
    {
        setElidedText({});
        return;
    }

    auto width = impl().fontMetrics->horizontalAdvance(impl().sourceText);
    if (width <= impl().widthLimit) {
        setElidedText(impl().sourceText);
        return;
    }

    auto decomposition = decomposePath(impl().sourceText);

    // Try remove some middle components
    std::vector<bool> skipDirs(decomposition.subdirs.size());
    auto mit = make_middle_iterator(skipDirs.begin(), skipDirs.end());
    while (mit.isValid()) {
        *mit++ = true;
        auto str = decomposition.combine(skipDirs);

        width = impl().fontMetrics->horizontalAdvance(str);
        if (width <= impl().widthLimit) {
            setElidedText(str);
            return;
        }
    }

    // Only name
    QString possibleResult;
    if (decomposition.subdirs.isEmpty() && decomposition.protocol.isEmpty()) {
        possibleResult = decomposition.name;
    } else {
        possibleResult = QStringLiteral("...") + decomposition.separator.value_or('/') + decomposition.name;
    }

    width = impl().fontMetrics->horizontalAdvance(possibleResult);
    if (width <= impl().widthLimit) {
        setElidedText(possibleResult);
        return;
    }

    // Shorten name
    possibleResult = "..." + decomposition.name;
    while (possibleResult.size() > 3) {
        width = impl().fontMetrics->horizontalAdvance(possibleResult);
        if (width <= impl().widthLimit) {
            setElidedText(possibleResult);
            return;
        } else {
            possibleResult.remove(3, 1);
        }
    }

    setElidedText("");
}

QString PathEliderDecomposition::combine(std::vector<bool> skipDirs) const
{
    auto separator = getSeparatorStr();
    auto result = protocol;

    if (!subdirs.isEmpty()) {
        bool needSep = false;
        bool ellipsisStarted = false;
        for (int i = 0; i < subdirs.size(); i++) {
            if (!skipDirs.empty() && skipDirs[i]) {
                if (ellipsisStarted) {
                    continue;
                } else {
                    if (needSep)
                        result += separator;
                    result += "...";
                    ellipsisStarted = true;
                    needSep = true;
                    continue;
                }
            }

            ellipsisStarted = false;

            if (needSep)
                result += separator;

            needSep = true;
            result += subdirs[i];
        }

        result += separator;
    }

    result += name;
    return result;
}
