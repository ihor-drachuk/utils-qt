#pragma once
#include <QValidator>
#include <QVariant>
#include <utils-cpp/pimpl.h>

class NumericalValidator : public QValidator {
    Q_OBJECT

    Q_PROPERTY(QVariant step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY(QVariant top READ top WRITE setTop NOTIFY topChanged)
    Q_PROPERTY(QVariant bottom READ bottom WRITE setBottom NOTIFY bottomChanged)

public:
    static void registerTypes();

    enum ValueRangeStatus {
        TopValue = 0,
        BottomValue,
        IntermediateValue
    };

    Q_ENUM(ValueRangeStatus)

    NumericalValidator(QObject* parent = nullptr);
    ~NumericalValidator() override;

    Q_INVOKABLE ValueRangeStatus isValue(const QString& input);

    State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;

    QVariant step() const;
    QVariant bottom() const;
    QVariant top() const;

public slots:
    void setStep(const QVariant& step);
    void setBottom(const QVariant& bottom);
    void setTop(const QVariant& top);

signals:
    void stepChanged(const QVariant& step);
    void bottomChanged(const QVariant& bottom);
    void topChanged(const QVariant& top);

private:
    int calculateDecimalPointPosition();
    void createStandartValidator();
    bool validateFixup(QString& input, const QChar decimalPoint) const;

private:
    DECLARE_PIMPL
};
