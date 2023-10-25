#pragma once
#include <QAbstractItemModel>
#include <QVariantList>
#include <utils-cpp/copy_move.h>
#include <utils-cpp/pimpl.h>

class CachedModel : public QAbstractItemModel
{
    Q_OBJECT
    NO_COPY_MOVE(CachedModel);
public:

    explicit CachedModel(QObject* parent = nullptr);
    ~CachedModel() override;

    void setSourceModel(QAbstractItemModel* srcModel);

public:
    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void dataAboutToBeChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

private:
    void reinit();
    void deinit();
    bool initable() const;
    bool ready() const;
    void verifyReadyFlag() const;
    void actualizeCache();

    void disconnectModel();
    void connectModel();
    void onModelDestroyed();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onBeforeLayoutChanged(const QList<QPersistentModelIndex>& parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void onAfterLayoutChanged(const QList<QPersistentModelIndex>& parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);

    void onRowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void onRowsInserted(const QModelIndex& parent, int first, int last);

    void onRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void onRowsRemoved(const QModelIndex& parent, int first, int last);

    void onColumnsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void onColumnsInserted(const QModelIndex& parent, int first, int last);

    void onColumnsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void onColumnsRemoved(const QModelIndex& parent, int first, int last);

    void onBeforeReset();
    void onAfterReset();

    void onRowsAboutToBeMoved( const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);
    void onRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);

    void onColumnsAboutToBeMoved( const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationColumn);
    void onColumnsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column);

private:
    DECLARE_PIMPL
};
