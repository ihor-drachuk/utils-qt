/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QIntValidator>

class NumericValidatorInt : public QIntValidator
{
    Q_OBJECT
public:
    static void registerTypes();

    NumericValidatorInt(QObject* parent = nullptr);

    State validate(QString& str, int& pos) const override;
    void fixup(QString& str) const override;
};
