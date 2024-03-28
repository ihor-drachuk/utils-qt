/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/Qml-Cpp/NumericalValidator.h>

#include <QQmlEngine>
#include <QGuiApplication>
#include <memory>
#include <cmath>

#include <UtilsQt/qvariant_traits.h>

struct NumericalValidator::impl_t
{
    QVariant step;
    QVariant rangeTop;
    QVariant rangeBottom;
    int decimals {0};

    std::unique_ptr<QValidator> standartValidator;
};

void NumericalValidator::registerTypes()
{
    qmlRegisterType<NumericalValidator>("UtilsQt", 1, 0, "NumericalValidator");
}

NumericalValidator::NumericalValidator(QObject* parent)
    : QValidator(parent)
{
    createImpl();
    qGuiApp->inputMethod()->locale().setDefault(QLocale::c());
}

NumericalValidator::~NumericalValidator()
{
}

NumericalValidator::ValueRangeStatus NumericalValidator::isValue(const QString& input)
{
    if (!impl().step.isValid() && !impl().rangeBottom.isValid() && !impl().rangeTop.isValid())
        return IntermediateValue;
    if (UtilsQt::QVariantTraits::isDouble(impl().step)) {
        QString top = QString::number(impl().rangeTop.toDouble());
        QString bottom = QString::number(impl().rangeBottom.toDouble());

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        validateFixup(top, impl().standartValidator->locale().decimalPoint()[0]);
        validateFixup(bottom, impl().standartValidator->locale().decimalPoint()[0]);
#else
        validateFixup(top, impl().standartValidator->locale().decimalPoint());
        validateFixup(bottom, impl().standartValidator->locale().decimalPoint());
#endif

        if (input == top)
            return TopValue;
        else if (input == bottom)
            return BottomValue;
        else
            return IntermediateValue;
    } else {
        if (input.toInt() >= impl().rangeTop.toInt())
            return TopValue;
        else if (input.toInt() <= impl().rangeBottom.toInt())
            return BottomValue;
        else
            return IntermediateValue;
    }
}

QString NumericalValidator::forceFixup(const QString& input) const
{
    QString text(input);
    if (text.isEmpty())
        text = QStringLiteral("0");

    fixup(text);
    return text;
}

QValidator::State NumericalValidator::validate(QString& input, int& pos) const
{
    if (input.isEmpty())
        input += '0';

    if (impl().decimals == 0) {
        input.remove(".");
        input.remove(",");
        if (input.toInt() > impl().rangeTop.toInt())
            return QValidator::State::Invalid;

    } else {
        auto decimalPoint = qGuiApp->inputMethod()->locale().decimalPoint();

        if (input.contains(',') && decimalPoint == '.') {
            return QValidator::State::Invalid;
        }

        int pointIndex = input.indexOf(",");
        if (pointIndex != -1)
            input[pointIndex] = '.';

        if (input.startsWith('0')) {
            int sizePrev = input.size();

            if (validateFixup(input, '.')) {
                int sizeNow = input.size();
                int diff = sizePrev - sizeNow;
                pos -= diff;
                pos = qBound(0, pos, input.size());
            }
        }

        if (impl().step.toDouble() < 1.0) {
            int sizePrev = input.size();
            if (validateFixup(input, '.')) {
                int sizeNow = input.size();
                int diff = sizePrev - sizeNow;
                pos -= diff;
                pos = qBound(0, pos, input.size());
            }
        }

        if (input == QStringLiteral("0")) {
            pos = 1;
        }
    }

    if (impl().standartValidator) {
        auto result = impl().standartValidator->validate(input, pos);
        return result;
    } else
        return State::Invalid;
}

void NumericalValidator::fixup(QString& input) const
{
    if (!impl().standartValidator)
        return;

    if (UtilsQt::QVariantTraits::isInteger(impl().step)) {
        impl().standartValidator->fixup(input);
    } else {
        validateFixup(input, '.');
    }
}

QVariant NumericalValidator::step() const
{
    return impl().step;
}

QVariant NumericalValidator::bottom() const
{
    return impl().rangeBottom;
}

QVariant NumericalValidator::top() const
{
    return impl().rangeTop;
}

void NumericalValidator::setStep(const QVariant& step)
{
    if (step == impl().step)
        return;

    assert(UtilsQt::QVariantTraits::isInteger(step) || UtilsQt::QVariantTraits::isDouble(step));
    if (UtilsQt::QVariantTraits::isDouble(step))
        assert(step.toDouble() > 0);

    impl().step = step;
    emit stepChanged(impl().step);
    createStandartValidator();
}

void NumericalValidator::setBottom(const QVariant& bottom)
{
    if (bottom == impl().rangeBottom)
        return;

    assert(UtilsQt::QVariantTraits::isInteger(bottom) || UtilsQt::QVariantTraits::isDouble(bottom));

    impl().rangeBottom = bottom;
    emit bottomChanged(impl().rangeBottom);
    createStandartValidator();
}

void NumericalValidator::setTop(const QVariant& top)
{
    if (top == impl().rangeTop)
        return;

    assert(UtilsQt::QVariantTraits::isInteger(top) || UtilsQt::QVariantTraits::isDouble(top));

    impl().rangeTop = top;
    emit topChanged(impl().rangeTop);
    createStandartValidator();
}

int NumericalValidator::calculateDecimalPointPosition()
{
    auto stepDoubleFormat = impl().step.toDouble();
    int decimalPosition = 0;
    while (true) {
        auto stepIntegerFormat = static_cast<unsigned int>(stepDoubleFormat);
        stepDoubleFormat -= stepIntegerFormat;
        if (stepDoubleFormat == 0)
            return decimalPosition;

        if (stepDoubleFormat == 10)
            return 10;

        decimalPosition++;
        stepDoubleFormat *= 10;
    }
}

void NumericalValidator::createStandartValidator()
{
    if (!impl().rangeBottom.isValid() || !impl().rangeTop.isValid() || !impl().step.isValid())
        return;

    if (UtilsQt::QVariantTraits::isDouble(step())) {
        impl().decimals = calculateDecimalPointPosition();
        impl().standartValidator.reset(new QDoubleValidator(impl().rangeBottom.toDouble(), impl().rangeTop.toDouble(), impl().decimals));
    } else {
        impl().decimals = 0;
        impl().standartValidator.reset(new QIntValidator(impl().rangeBottom.toInt(), impl().rangeTop.toInt()));
    }
}

bool NumericalValidator::validateFixup(QString& input, const QChar decimalPoint) const
{
    bool result = false;

    if (*input.rbegin() != decimalPoint) {
        if (input.toDouble() < impl().rangeBottom.toDouble())
            input = QString::number(impl().rangeBottom.toDouble());

        if (input.toDouble() > impl().rangeTop.toDouble())
            input = QString::number(impl().rangeTop.toDouble());
    }

    if (input.isEmpty() || input == QString(decimalPoint)) {
        input = QString::number(impl().rangeBottom.toDouble());
    } else {
        int commaDelta = 0;
        auto it = input.begin();
        for ( ; it != input.end(); it++) {
            if (*it == decimalPoint) {
                commaDelta = 1;
                break;
            }

            if (*it != '0') break;
        }

        int count = it - input.begin();
        int delta = (it == input.end()) ? 1 : 0;
        int toRemove = count - delta - commaDelta;

        if (toRemove) {
            input.remove(0, toRemove);
            result = true;
        }

        if (impl().step.toDouble() < 1.0) {
            auto index = input.indexOf(decimalPoint);
            if (index != -1) {
                auto comlectFractionNumber = input.split(decimalPoint);
                if (comlectFractionNumber.at(1).size() > impl().decimals) {
                    auto fractionNumberStr = comlectFractionNumber.at(1);
                    int digitAfterDecimals = fractionNumberStr[impl().decimals].digitValue() ;
                    fractionNumberStr.resize(impl().decimals);
                    int fractionNumber = fractionNumberStr.toUInt();
                    if (digitAfterDecimals > 5) {
                        fractionNumber++;
                        if (fractionNumber >= std::pow(10, impl().decimals)) {
                            QString integerNumberStr(QString::number(comlectFractionNumber.at(0).toInt()+1));
                            fractionNumber--;
                            fractionNumber /= lround(std::pow(10, impl().decimals));
                            input = integerNumberStr;
                        } else {
                            input = comlectFractionNumber.at(0);
                        }

                    } else {
                        input = comlectFractionNumber.at(0);
                    }

                    if (fractionNumber != 0) {
                        input += decimalPoint;
                        for (;;) {
                            if (QString::number(fractionNumber).size() <= impl().decimals)
                                break;

                            fractionNumber /= 10;
                        }

                        QString fractionNumberStr(QString::number(fractionNumber));

                        while(fractionNumberStr.size() < impl().decimals) {
                            fractionNumberStr.push_front('0');
                        }

                        input += fractionNumberStr;
                    }
                }
            }
        }
    }

    impl().standartValidator->fixup(input);

    return result;
}
