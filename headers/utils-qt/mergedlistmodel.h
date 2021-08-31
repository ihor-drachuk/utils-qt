#pragma once
#include <QAbstractListModel>
#include <utils-cpp/pimpl.h>

class MergedListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant joinRole1 READ joinRole1 WRITE setJoinRole1 NOTIFY joinRole1Changed) // int or string
    Q_PROPERTY(QVariant joinRole2 READ joinRole2 WRITE setJoinRole2 NOTIFY joinRole2Changed) // int or string
    Q_PROPERTY(QAbstractListModel* model1 READ model1 WRITE setModel1 NOTIFY model1Changed)
    Q_PROPERTY(QAbstractListModel* model2 READ model2 WRITE setModel2 NOTIFY model2Changed)
public:
    explicit MergedListModel(QObject* parent = nullptr);
    ~MergedListModel() override;

    static void registerTypes();

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

// --- Properties support ---
public:
    QVariant joinRole1() const;
    QVariant joinRole2() const;
    QAbstractListModel* model1() const;
    QAbstractListModel* model2() const;

public slots:
    void setJoinRole1(const QVariant& value);
    void setJoinRole2(const QVariant& value);
    void setModel1(QAbstractListModel* value);
    void setModel2(QAbstractListModel* value);

signals:
    void joinRole1Changed(const QVariant& joinRole1);
    void joinRole2Changed(const QVariant& joinRole2);
    void model1Changed(QAbstractListModel* model1);
    void model2Changed(QAbstractListModel* model2);
// --- ---

private:
    void selfCheckModel(int idx) const;
    void selfCheck() const;
    bool initable() const;
    void init();
    void deinit();

    void connectModel(int number);

    void onModelDestroyed(int number);
    void onDataChanged(int number, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onBeforeInserted(int number, const QModelIndex& parent, int first, int last);
    void onAfterInserted(int number, const QModelIndex& parent, int first, int last);
    void onBeforeRemoved(int number, const QModelIndex& parent, int first, int last);
    void onAfterRemoved(int number, const QModelIndex& parent, int first, int last);
    void onBeforeReset(int number);
    void onAfterReset(int number);

private:
    DECLARE_PIMPL
};
