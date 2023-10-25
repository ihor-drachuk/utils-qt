#include <UtilsQt/CachedModel.h>

#include <iterator>
#include <QHash>
#include <QList>
#include <UtilsQt/convertcontainer.h>

namespace {

using Cell = QHash<int, QVariant>; // role-value
using Column = QList<Cell>;  // y
using Matrix = QList<Column>; // x

template<typename Iter>
void move(Iter first, Iter last, Iter dest)
{

}

} // namespace

struct CachedModel::impl_t
{
    QAbstractItemModel* srcModel {};
    QList<QMetaObject::Connection> modelConnections;
    bool ready {};

    Matrix cache; // cache[x]  [y]  [role]
                  // cache[col][row][role]
};

CachedModel::CachedModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    createImpl();
}

CachedModel::~CachedModel()
{
}

void CachedModel::setSourceModel(QAbstractItemModel* srcModel)
{
    beginResetModel();
    deinit();
    impl().srcModel = srcModel;
    reinit();
    endResetModel();
}

QModelIndex CachedModel::index(int row, int column, const QModelIndex& parent) const
{
    assert(parent == QModelIndex());
    return ready() ? impl().srcModel->index(row, column, parent) : QModelIndex();
}

QModelIndex CachedModel::parent(const QModelIndex& child) const
{
    return ready() ? impl().srcModel->parent(child) : QModelIndex();
}

int CachedModel::rowCount(const QModelIndex& parent) const
{
    assert(parent == QModelIndex());
    return ready() ? impl().srcModel->rowCount(parent) : 0;
}

int CachedModel::columnCount(const QModelIndex& parent) const
{
    assert(parent == QModelIndex());
    return ready() ? impl().srcModel->columnCount(parent) : 0;
}

QVariant CachedModel::data(const QModelIndex& index, int role) const
{
    if (!ready())
        return {};

    return impl().cache[index.column()][index.row()][role];
}

bool CachedModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!ready())
        return false;

    return impl().srcModel->setData(index, value, role);
}

QHash<int, QByteArray> CachedModel::roleNames() const
{
    return ready() ? impl().srcModel->roleNames() : QHash<int, QByteArray>();
}

void CachedModel::reinit()
{
    deinit();

    if (!initable())
        return;

    actualizeCache();
    connectModel();
    impl().ready = true;
}

void CachedModel::deinit()
{
    if (!impl().ready)
        return;

    impl().ready = false;
    disconnectModel();
    actualizeCache();
}

bool CachedModel::initable() const
{
    return impl().srcModel != nullptr;
}

bool CachedModel::ready() const
{
    return impl().ready;
}

void CachedModel::actualizeCache()
{
    // Clear cache
    impl().cache.clear();

    if (!impl().srcModel)
        return;

    // Build new cache
    const auto roles = roleNames().keys();
    const auto cols = impl().srcModel->columnCount({});
    const auto rows = impl().srcModel->rowCount({});

    for (int col = 0; col < cols; col++) {
        Column column;

        for (int row = 0; row < rows; row++) {
            Cell cell;

            for (auto r : roles) {
                const QVariant value = impl().srcModel->data(impl().srcModel->index(row, col, {}), r);
                cell.insert(r, value);
            }

            column.append(cell);
        }

        impl().cache.append(column);
    }
}

void CachedModel::disconnectModel()
{
    for (const auto& x : qAsConst(impl().modelConnections))
        QObject::disconnect(x);

    impl().modelConnections.clear();
}

void CachedModel::connectModel()
{
    using namespace std::placeholders;

    assert(impl().modelConnections.isEmpty());

    auto save = [this](QMetaObject::Connection c) { impl().modelConnections.append(c); };

    save(QObject::connect(impl().srcModel, &QAbstractItemModel::destroyed,                this, &CachedModel::onModelDestroyed));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::dataChanged,              this, &CachedModel::onDataChanged));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::headerDataChanged,        this, &CachedModel::headerDataChanged)); // Pass/transit
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsAboutToBeInserted,    this, &CachedModel::onRowsAboutToBeInserted));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsInserted,             this, &CachedModel::onRowsInserted));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsAboutToBeRemoved,     this, &CachedModel::onRowsAboutToBeRemoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsRemoved,              this, &CachedModel::onRowsRemoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsAboutToBeInserted, this, &CachedModel::onColumnsAboutToBeInserted));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsInserted,          this, &CachedModel::onColumnsInserted));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsAboutToBeRemoved,  this, &CachedModel::onColumnsAboutToBeRemoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsRemoved,           this, &CachedModel::onColumnsRemoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::modelAboutToBeReset,      this, &CachedModel::onBeforeReset));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::modelReset,               this, &CachedModel::onAfterReset));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::layoutAboutToBeChanged,   this, &CachedModel::onBeforeLayoutChanged));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::layoutChanged,            this, &CachedModel::onAfterLayoutChanged));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsAboutToBeMoved,       this, &CachedModel::onRowsAboutToBeMoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::rowsMoved,                this, &CachedModel::onRowsMoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsAboutToBeMoved,    this, &CachedModel::onColumnsAboutToBeMoved));
    save(QObject::connect(impl().srcModel, &QAbstractItemModel::columnsMoved,             this, &CachedModel::onColumnsMoved));
}

void CachedModel::onModelDestroyed()
{
    setSourceModel(nullptr);
}

void CachedModel::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    emit dataAboutToBeChanged(topLeft, bottomRight, roles);

    const auto actualRoles = roles.empty() ? UtilsQt::toVector(roleNames().keys()) : roles;

    for (int col = topLeft.column(); col <= bottomRight.column(); col++) {
        for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
            for (auto r : actualRoles) {
                const QVariant value = impl().srcModel->data(impl().srcModel->index(row, col, {}), r);
                impl().cache[col][row][r] = value;
            }
        }
    }

    emit dataChanged(topLeft, bottomRight, roles);
}

void CachedModel::onBeforeLayoutChanged(const QList<QPersistentModelIndex>& parents, LayoutChangeHint hint)
{
    emit layoutAboutToBeChanged(parents, hint);
}

void CachedModel::onAfterLayoutChanged(const QList<QPersistentModelIndex>& parents, LayoutChangeHint hint)
{
    reinit();
    emit layoutChanged(parents, hint);
}

void CachedModel::onRowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());
    beginInsertRows(parent, first, last);
}

void CachedModel::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());
    // ...
    endInsertRows();
}

void CachedModel::onRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());
    beginRemoveRows(parent, first, last);
}

void CachedModel::onRowsRemoved(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());

    for (int col = 0; col < impl().cache.size(); col++) {
        auto& c = impl().cache[col];
        c.erase(c.begin() + first, c.begin() + last + 1);
    }

    endRemoveRows();
}

void CachedModel::onColumnsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());
    beginInsertColumns(parent, first, last);
}

void CachedModel::onColumnsInserted(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());

    std::insert_iterator<QList<Column>> inserter(impl().cache, impl().cache.begin() + first);

    const auto roles = roleNames().keys();
    const auto rows = impl().srcModel->rowCount({});

    for (int col = first; col <= last; col++) {
        Column column;

        for (int row = 0; row < rows; row++) {
            Cell cell;

            for (auto r : roles) {
                const QVariant value = impl().srcModel->data(impl().srcModel->index(row, col, {}), r);
                cell.insert(r, value);
            }

            column.append(cell);
        }

        *inserter++ = column;
    }

    endInsertColumns();
}

void CachedModel::onColumnsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());
    beginRemoveColumns(parent, first, last);
}

void CachedModel::onColumnsRemoved(const QModelIndex& parent, int first, int last)
{
    assert(parent == QModelIndex());
    const auto bg = impl().cache.begin();
    impl().cache.erase(bg + first, bg + last + 1);
    endRemoveColumns();
}

void CachedModel::onBeforeReset()
{
    beginResetModel();
}

void CachedModel::onAfterReset()
{
    reinit();
    endResetModel();
}

void CachedModel::onRowsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    assert(sourceParent == QModelIndex());
    assert(destinationParent == QModelIndex());
    beginMoveRows(sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);
}

void CachedModel::onRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    assert(parent == QModelIndex());
    assert(destination == QModelIndex());

    for (auto& column : impl().cache) {
        const auto bg = column.begin();
        // std::move(bg + start, bg + end + 1, bg + row);
        // ...
    }

    endMoveRows();
}

void CachedModel::onColumnsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationColumn)
{
    assert(sourceParent == QModelIndex());
    assert(destinationParent == QModelIndex());
    beginMoveColumns(sourceParent, sourceStart, sourceEnd, destinationParent, destinationColumn);
}

void CachedModel::onColumnsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column)
{
    assert(parent == QModelIndex());
    assert(destination == QModelIndex());
    const auto bg = impl().cache.begin();
    //std::move(bg + start, bg + end + 1, bg + column);
    // ...
    endMoveColumns();
}
