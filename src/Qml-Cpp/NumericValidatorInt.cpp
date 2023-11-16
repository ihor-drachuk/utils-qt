/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/NumericValidatorInt.h>

#include <QQmlEngine>

void NumericValidatorInt::registerTypes()
{
    qmlRegisterType<NumericValidatorInt>("UtilsQt", 1, 0, "NumericValidatorInt");
}

NumericValidatorInt::NumericValidatorInt(QObject* parent)
    : QIntValidator(parent)
{
}

QValidator::State NumericValidatorInt::validate(QString& str, int& /*pos*/) const
{
    assert(bottom() < top());

    // Intermediate:
    //  ..if empty
    if (str.isEmpty()) return State::Intermediate;

    //  ..if number is starting with '-', but for now it's only '-'
    if (str == '-' && bottom() < 0) return State::Intermediate;

    // Is it number? Has leading zeros? Has only zeroes?
    int i = 0;
    int zeros = 0;
    bool zerosCounting = true;
    bool onlyZeros = true;
    for (const auto& x : qAsConst(str)) {
        const bool allowMinus = (i == 0 && bottom() < 0);
        const bool allowableChar = x.isDigit() || (allowMinus && x == '-');

        if (!allowableChar)
            return State::Invalid;

        zerosCounting &= (x == '0') || (allowMinus && x == '-');
        zeros += zerosCounting && (x == '0') ? 1 : 0;
        onlyZeros &= (x == '0') || (allowMinus && x == '-');

        i++;
    }

    // Is negative?
    bool isNegative = str.startsWith('-');

    // Convert to int and check range
    bool ok;
    auto num = str.toInt(&ok, 10);
    assert(ok && "Can't convert string to int");

    if (num < bottom()) {
        str = QString::number(bottom());
        return State::Acceptable;

    } else if (num > top()) {
        str = QString::number(top());
        return State::Acceptable;
    }

    // Remove leading zeros (avoid '00123', '-04')
    if (str.size() > 1 && zeros > 0) {
        if (onlyZeros)
            zeros--;

        str.remove(isNegative ? 1 : 0, zeros);
    }

    return State::Acceptable;
}

void NumericValidatorInt::fixup(QString& str) const
{
    if (str.isEmpty() || str == "-") {
        if (bottom() <= 0 && top() >= 0) {
            str = "0";
        } else if (bottom() > 0) {
            str = QString::number(bottom());
        } else if (top() < 0) {
            str = QString::number(top());
        }

        return;
    }

    QIntValidator::fixup(str);
}
