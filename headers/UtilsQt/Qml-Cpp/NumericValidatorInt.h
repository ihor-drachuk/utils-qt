#pragma once
#include <QIntValidator>

class NumericValidatorInt : public QIntValidator
{
    Q_OBJECT
public:
    static void registerTypes();

    NumericValidatorInt(QObject* parent = nullptr);
    ~NumericValidatorInt() override;

    State validate(QString& str, int& pos) const override;
    void fixup(QString& str) const override;
};
