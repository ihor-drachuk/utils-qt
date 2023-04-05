#pragma once
#include <QAbstractItemModel>
#include <QVariantList>
#include <functional>
#include <variant>
#include <utils-cpp/copy_move.h>
#include <utils-cpp/pimpl.h>

class AugmentedModel : public QAbstractItemModel
{
    Q_OBJECT
    NO_COPY_MOVE(AugmentedModel);
public:
    using RoleUpdater = std::function<void(const QModelIndex&, const QModelIndex&)>;
    using Calculator = std::function<QVariant (const QVariantList&)>;
    using Role = std::variant<QString, int>;

    explicit AugmentedModel(QObject* parent = nullptr);
    ~AugmentedModel() override;

    void setSourceModel(QAbstractItemModel* srcModel);

    RoleUpdater addCalculatedRole(const QString& name,
                                  const QList<Role>& sourceRoles,
                                  const Calculator& calculator);

    void updateAllCalculatedRoles();

public:
    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

private:
    void reinit();
    void deinit();
    bool initable() const;
    bool ready() const;
    void verifyReadyFlag() const;
    void actualizeCache();
    bool isCalculatedRole(int role) const;
    void updateCalculatedRole(const QModelIndex& topLeft, const QModelIndex& bottomRight, int role);

    void disconnectModel();
    void connectModel();
    void onModelDestroyed();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onBeforeReset();
    void onAfterReset();
    void onBeforeLayoutChanged(const QList<QPersistentModelIndex>& parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void onAfterLayoutChanged(const QList<QPersistentModelIndex>& parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);


private:
    struct SourceRole;
    struct CalculatedRoleDetails;
    using CalculatedRoleDetailsPtr = std::shared_ptr<CalculatedRoleDetails>;
    DECLARE_PIMPL
};
