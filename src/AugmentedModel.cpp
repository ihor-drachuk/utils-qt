/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/AugmentedModel.h>

#include <QMap>
#include <QHash>
#include <QSet>
#include <algorithm>

struct AugmentedModel::SourceRole
{
    Role roleId;
    int role {-1}; // src, cached
};

struct AugmentedModel::CalculatedRoleDetails
{
    AugmentedModel* parent {};
    QList<SourceRole> sourceRoles;
    QString name;
    int role {-1}; // own, cached
    Calculator calculator;
};

struct AugmentedModel::impl_t
{
    QAbstractItemModel* srcModel {};
    QList<QMetaObject::Connection> modelConnections;
    QMap<QString, int> srcModelRolesMap; // cached

    QList<CalculatedRoleDetailsPtr> calculatedRoles;
    QSet<int> sourceRolesCache; // cached
    QMultiHash<int, int> sourceRoleToCalculated; // cached
    QHash<int, int> roleToCalcRolesIndex; // cached
    QHash<int, QByteArray> cachedRoles; // cached

    bool ready {};
};

AugmentedModel::AugmentedModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    createImpl();
}

AugmentedModel::~AugmentedModel()
{
}

void AugmentedModel::reinit()
{
    deinit();

    if (!initable())
        return;

    actualizeCache();
    connectModel();
    impl().ready = true;
}

void AugmentedModel::deinit()
{
    if (!impl().ready)
        return;

    impl().ready = false;
    disconnectModel();
    actualizeCache();
}

bool AugmentedModel::initable() const
{
    return impl().srcModel != nullptr;
}

bool AugmentedModel::ready() const
{
    return impl().ready;
}

void AugmentedModel::actualizeCache()
{
    impl().srcModelRolesMap.clear();
    impl().roleToCalcRolesIndex.clear();
    impl().cachedRoles = impl().srcModel ? impl().srcModel->roleNames() : QHash<int, QByteArray>();
    impl().sourceRolesCache.clear();
    impl().sourceRoleToCalculated.clear();

    if (!impl().srcModel)
        return;

    // srcModelRolesMap
    const auto roleNames = impl().srcModel->roleNames();
    for (auto it = roleNames.cbegin(); it != roleNames.cend(); it++)
        impl().srcModelRolesMap.insert(QString::fromUtf8(it.value()), it.key());

    // calculatedRoles, roleToCalcRolesIndex, cachedRoles
    auto srcRoleIds = impl().srcModel->roleNames().keys();
    srcRoleIds.append(Qt::UserRole);
    auto nextRoleId = *std::max_element(srcRoleIds.cbegin(), srcRoleIds.cend()) + 1;
    for (int i = 0; i < impl().calculatedRoles.size(); i++) {
        auto& calcRole = *impl().calculatedRoles[i];
        const auto role = nextRoleId++;
        calcRole.role = role;
        impl().roleToCalcRolesIndex.insert(role, i);

        for (auto& x : impl().calculatedRoles[i]->sourceRoles) {
            x.role = std::holds_alternative<QString>(x.roleId) ?
                         impl().srcModelRolesMap.value(std::get<QString>(x.roleId)) :
                         std::get<int>(x.roleId);
            impl().sourceRolesCache.insert(x.role);
            impl().sourceRoleToCalculated.insert(x.role, role);
            assert(x.role >= 0);
        }

        impl().cachedRoles.insert(calcRole.role, calcRole.name.toUtf8());
    }
}

bool AugmentedModel::isCalculatedRole(int role) const
{
    return (impl().roleToCalcRolesIndex.count(role));
}

void AugmentedModel::updateCalculatedRole(const QModelIndex& topLeft,
                                          const QModelIndex& bottomRight,
                                          int role)
{
    assert(ready());
    emit dataChanged(topLeft, bottomRight, {role});
}

void AugmentedModel::disconnectModel()
{
    for (const auto& x : qAsConst(impl().modelConnections))
        QObject::disconnect(x);

    impl().modelConnections.clear();
}

void AugmentedModel::connectModel()
{
    using namespace std::placeholders;

    assert(impl().modelConnections.isEmpty());

    auto save = [this](QMetaObject::Connection c) { impl().modelConnections.append(c); };

    save(QObject::connect(impl().srcModel, &QAbstractItemModel::destroyed,                this, &AugmentedModel::onModelDestroyed));          // L
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::dataChanged,              this, &AugmentedModel::onDataChanged));             // L
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::headerDataChanged,        this, &AugmentedModel::headerDataChanged));         // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsAboutToBeInserted,    this, &AugmentedModel::rowsAboutToBeInserted));     // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsInserted,             this, &AugmentedModel::rowsInserted));              // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsAboutToBeRemoved,     this, &AugmentedModel::rowsAboutToBeRemoved));      // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsRemoved,              this, &AugmentedModel::rowsRemoved));               // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsAboutToBeInserted, this, &AugmentedModel::columnsAboutToBeInserted));  // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsInserted,          this, &AugmentedModel::columnsInserted));           // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsAboutToBeRemoved,  this, &AugmentedModel::columnsAboutToBeRemoved));   // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsRemoved,           this, &AugmentedModel::columnsRemoved));            // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::modelAboutToBeReset,      this, &AugmentedModel::onBeforeReset));             // L
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::modelReset,               this, &AugmentedModel::onAfterReset));              // L
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::layoutAboutToBeChanged,   this, &AugmentedModel::onBeforeLayoutChanged));     // L
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::layoutChanged,            this, &AugmentedModel::onAfterLayoutChanged));      // L
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsAboutToBeMoved,       this, &AugmentedModel::rowsAboutToBeMoved));        // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsMoved,                this, &AugmentedModel::rowsMoved));                 // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsAboutToBeMoved,    this, &AugmentedModel::columnsAboutToBeMoved));     // Pass
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsMoved,             this, &AugmentedModel::columnsMoved));              // Pass
}

void AugmentedModel::onModelDestroyed()
{
    setSourceModel(nullptr);
}

void AugmentedModel::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    auto changedRolesSet = QSet<int>(roles.cbegin(), roles.cend());
    const auto intersection = changedRolesSet & impl().sourceRolesCache;

    for (auto srcRole : intersection) {
        const auto calcRoles = impl().sourceRoleToCalculated.values(srcRole);
        for (auto calcRole : calcRoles) {
            const auto calcRoleIdx = impl().roleToCalcRolesIndex.value(calcRole);
            changedRolesSet.insert(impl().calculatedRoles.at(calcRoleIdx)->role);
        }
    }

    QVector changedRoles(changedRolesSet.cbegin(), changedRolesSet.cend());
    std::sort(changedRoles.begin(), changedRoles.end());

    emit dataChanged(topLeft, bottomRight, changedRoles);
}

void AugmentedModel::onBeforeReset()
{
    beginResetModel();
}

void AugmentedModel::onAfterReset()
{
    reinit();
    endResetModel();
}

void AugmentedModel::onBeforeLayoutChanged(const QList<QPersistentModelIndex>& parents, LayoutChangeHint hint)
{
    emit layoutAboutToBeChanged(parents, hint);
}

void AugmentedModel::onAfterLayoutChanged(const QList<QPersistentModelIndex>& parents, LayoutChangeHint hint)
{
    reinit();
    emit layoutChanged(parents, hint);
}

void AugmentedModel::setSourceModel(QAbstractItemModel* srcModel)
{
    beginResetModel();
    deinit();
    impl().srcModel = srcModel;
    reinit();
    endResetModel();
}

AugmentedModel::RoleUpdater AugmentedModel::addCalculatedRole(const QString& name, const QList<Role>& sourceRoles, const Calculator& calculator)
{
    assert(!ready() && "Set source model AFTER adding calculated roles!");

    auto calculatedRole = std::make_shared<CalculatedRoleDetails>();
    calculatedRole->parent = this;
    calculatedRole->name = name;
    calculatedRole->calculator = calculator;

    for (const auto& x : sourceRoles) {
        SourceRole srcRole;
        srcRole.roleId = x;
        calculatedRole->sourceRoles.append(srcRole);
    }

    impl().calculatedRoles.append(calculatedRole);

    auto result = [weakDetails = std::weak_ptr<CalculatedRoleDetails>(calculatedRole)](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
        auto details = weakDetails.lock();
        assert(details);
        assert(details->parent);
        details->parent->updateCalculatedRole(topLeft, bottomRight, details->role);
    };

    return result;
}

void AugmentedModel::updateAllCalculatedRoles()
{
    if (impl().calculatedRoles.isEmpty())
        return;

    QVector<int> affectedRoles;
    for (const auto& x : qAsConst(impl().calculatedRoles))
        affectedRoles.append(x->role);

    emit dataChanged(index(0, 0, {}), index(rowCount({}) - 1, columnCount({}) - 1, {}), affectedRoles);
}

QModelIndex AugmentedModel::index(int row, int column, const QModelIndex& parent) const
{
    return ready() ? impl().srcModel->index(row, column, parent) : QModelIndex();
}

QModelIndex AugmentedModel::parent(const QModelIndex& child) const
{
    return ready() ? impl().srcModel->parent(child) : QModelIndex();
}

int AugmentedModel::rowCount(const QModelIndex& parent) const
{
    return ready() ? impl().srcModel->rowCount(parent) : 0;
}

int AugmentedModel::columnCount(const QModelIndex& parent) const
{
    return ready() ? impl().srcModel->columnCount(parent) : 0;
}

QVariant AugmentedModel::data(const QModelIndex& index, int role) const
{
    if (!ready())
        return {};

    if (isCalculatedRole(role)) {
        auto crIndex = impl().roleToCalcRolesIndex.value(role);

        QVariantList srcValues;
        for (const auto& x : qAsConst(impl().calculatedRoles.at(crIndex)->sourceRoles)) {
            const auto xValue = impl().srcModel->data(index, x.role);
            srcValues.append(xValue);
        }

        auto result = impl().calculatedRoles.at(crIndex)->calculator(srcValues);
        return result;
    } else {
        return impl().srcModel->data(index, role);
    }
}

bool AugmentedModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!ready())
        return false;

    if (isCalculatedRole(role)) {
        assert(!"setData on calculated role is not supported!");
        return false;
    } else {
        return impl().srcModel->setData(index, value, role);
    }
}

QHash<int, QByteArray> AugmentedModel::roleNames() const
{
    return ready() ? impl().cachedRoles : QHash<int, QByteArray>();
}
