#include <UtilsQt/Qml-Cpp/ListModelItemProxy.h>

#include <optional>
#include <cassert>
#include <QMap>
#include <QQmlEngine>
#include <utils-cpp/scoped_guard.h>

struct ListModelItemProxy::impl_t
{
    QQmlPropertyMap* propertyMap { nullptr };
    QAbstractItemModel* model { nullptr };
    int index { -1 };
    bool ready { false };
    bool keepIndexTrack { true };

    bool isChanging { false };
    std::optional<int> expectedIndex;

    QMap<QString, int> rolesCache;
};


void ListModelItemProxy::registerTypes()
{
    qRegisterMetaType<QAbstractItemModel*>("QAbstractItemModel*");
    qRegisterMetaType<QQmlPropertyMap*>("QQmlPropertyMap*");
    qmlRegisterType<ListModelItemProxy>("UtilsQt", 1, 0, "ListModelItemProxy");
}


ListModelItemProxy::ListModelItemProxy(QObject* parent)
    : QObject(parent)
{
    createImpl();
}

ListModelItemProxy::~ListModelItemProxy()
{
    delete impl().propertyMap;
}

QAbstractItemModel* ListModelItemProxy::model() const
{
    return impl().model;
}

int ListModelItemProxy::index() const
{
    return impl().index;
}

bool ListModelItemProxy::ready() const
{
    return impl().ready;
}

QQmlPropertyMap* ListModelItemProxy::propertyMap() const
{
    return impl().propertyMap;
}

bool ListModelItemProxy::keepIndexTrack() const
{
    return impl().keepIndexTrack;
}

void ListModelItemProxy::setModel(QAbstractItemModel* value)
{
    if (!qobject_cast<QAbstractItemModel*>(value)) {
        value = nullptr;
    }

    if (impl().model == value)
        return;

    if (impl().model) {
        QObject::disconnect(impl().model, nullptr, this, nullptr);
    }

    assert(!value || value->columnCount() == 1);

    impl().model = value;
    reload();
    emit modelChanged(impl().model);

    if (impl().model) {
        QObject::connect(impl().model, &QAbstractItemModel::rowsAboutToBeInserted, this, &ListModelItemProxy::onRowsInsertedBefore);
        QObject::connect(impl().model, &QAbstractItemModel::rowsInserted, this, &ListModelItemProxy::onRowsInserted);
        QObject::connect(impl().model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ListModelItemProxy::onRowsRemovedBefore);
        QObject::connect(impl().model, &QAbstractItemModel::rowsRemoved, this, &ListModelItemProxy::onRowsRemoved);
        QObject::connect(impl().model, &QAbstractItemModel::modelReset, this, &ListModelItemProxy::onModelReset);
        QObject::connect(impl().model, &QAbstractItemModel::rowsMoved, this, &ListModelItemProxy::onRowsMoved);
        QObject::connect(impl().model, &QAbstractItemModel::dataChanged, this, &ListModelItemProxy::onDataChanged);

        QObject::connect(impl().model, &QAbstractItemModel::rowsInserted, this, std::bind(&ListModelItemProxy::countChanged, this, std::bind(&ListModelItemProxy::count, this)));
        QObject::connect(impl().model, &QAbstractItemModel::rowsRemoved, this, std::bind(&ListModelItemProxy::countChanged, this, std::bind(&ListModelItemProxy::count, this)));
        QObject::connect(impl().model, &QAbstractItemModel::modelReset, this, std::bind(&ListModelItemProxy::countChanged, this, std::bind(&ListModelItemProxy::count, this)));

        QObject::connect(impl().model, &QAbstractItemModel::layoutAboutToBeChanged, this, &ListModelItemProxy::onLayoutAboutToBeChanged);
        QObject::connect(impl().model, &QAbstractItemModel::layoutChanged, this, &ListModelItemProxy::onLayoutChanged);

        QObject::connect(impl().model, &QAbstractItemModel::columnsAboutToBeInserted, this, &ListModelItemProxy::onColumnsAboutToBeInserted);
        QObject::connect(impl().model, &QAbstractItemModel::columnsAboutToBeRemoved, this, &ListModelItemProxy::onColumnsAboutToBeRemoved);
        QObject::connect(impl().model, &QAbstractItemModel::columnsAboutToBeMoved, this, &ListModelItemProxy::onColumnsAboutToBeMoved);
    }
}

void ListModelItemProxy::setIndex(int value)
{
    if (impl().index == value)
        return;

    if (impl().expectedIndex.has_value() && impl().expectedIndex.value() == value && impl().isChanging)
        return;

    impl().index = value;
    reload();
    emit indexChanged(impl().index);
}

void ListModelItemProxy::setReady(bool value)
{
    if (impl().ready == value)
        return;

    impl().ready = value;
    emit readyChanged(impl().ready);
}

bool ListModelItemProxy::isValidIndex() const
{
    if (!impl().model)
        return false;

    if (impl().index == -1 || impl().index >= impl().model->rowCount())
        return false;

    return true;
}

void ListModelItemProxy::reload()
{
    auto _sg = CreateScopedGuard([this](){
        emit changed();
    });

    setReady(false);

    setPropertyMap(new QQmlPropertyMap());
    impl().rolesCache.clear();

    if (!impl().model || !isValidIndex())
        return;

    if (impl().index >= impl().model->rowCount())
        return;

    auto map = new QQmlPropertyMap();

    auto roles = impl().model->roleNames();
    auto it = roles.begin();
    const auto itEnd = roles.end();

    while (it != itEnd) {
        auto role = it.key();
        auto key = QString::fromLatin1(it.value());
        impl().rolesCache.insert(key, role);
        auto value = impl().model->data(impl().model->index(impl().index, 0), role);
        map->insert(key, value);
        it++;
    }

    QObject::connect(map, &QQmlPropertyMap::valueChanged, this, &ListModelItemProxy::onValueChanged);
    setPropertyMap(map);
    setReady(true);
    //emit changed(); -- by scoped guard
}

void ListModelItemProxy::reloadRoles(const QVector<int>& affectedRoles)
{
    auto roles = impl().model->roleNames();
    auto it = roles.begin();
    const auto itEnd = roles.end();

    while (it != itEnd) {
        auto role = it.key();

        if (!affectedRoles.isEmpty() && !affectedRoles.contains(role)) {
            it++;
            continue;
        }

        auto key = QString::fromLatin1(it.value());
        auto value = impl().model->data(impl().model->index(impl().index, 0), role);

        if (impl().propertyMap->value(key) != value) {
            impl().propertyMap->insert(key, value);
        }

        it++;
    }

    emit changed();
}

void ListModelItemProxy::onValueChanged(const QString& key, const QVariant& value)
{
    assert(impl().ready);
    assert(isValidIndex());
    impl().model->setData(impl().model->index(impl().index, 0), value, impl().rolesCache.value(key));
}

void ListModelItemProxy::onRowsInserted(const QModelIndex& /*parent*/, int first, int last)
{
    if (!isValidIndex())
        return;

    if (first > impl().index)
        return;

    impl().isChanging = false;

    if (impl().keepIndexTrack) {
        impl().index = impl().expectedIndex.value();
        impl().expectedIndex.reset();
        emit indexChanged(impl().index);

    } else {
        auto shift = last - first + 1;
        emit suggestedNewIndex(impl().index, impl().index + shift);

        reloadRoles();
    }
}

void ListModelItemProxy::onRowsInsertedBefore(const QModelIndex& /*parent*/, int first, int last)
{
    if (!isValidIndex())
        return;

    if (first > impl().index)
        return;

    impl().isChanging = true;

    if (impl().keepIndexTrack) {
        auto shift = last - first + 1;
        auto suggestedIndex = impl().index + shift;
        impl().expectedIndex = suggestedIndex;
    }
}

void ListModelItemProxy::onRowsRemoved(const QModelIndex& /*parent*/, int first, int last)
{
    if (impl().index == -1)
        return;

    if (first > impl().index)
        return;

    impl().isChanging = false;

    auto isRemoved = (impl().index >= first && impl().index <= last);
    auto shift = last - first + 1;
    auto suggestedIndex = impl().index - shift;

    if (isRemoved)
        emit removed();

    if (impl().keepIndexTrack && !isRemoved) {
        impl().index = impl().expectedIndex.value();
        impl().expectedIndex.reset();
        emit indexChanged(impl().index);

    } else {
        emit suggestedNewIndex(impl().index, isRemoved ? -1 : suggestedIndex);

        if (impl().index >= impl().model->rowCount()) {
            reload();
        } else {
            reloadRoles();
        }
    }
}

void ListModelItemProxy::onRowsRemovedBefore(const QModelIndex& /*parent*/, int first, int last)
{
    if (impl().index == -1)
        return;

    if (first > impl().index)
        return;

    impl().isChanging = true;

    if (impl().keepIndexTrack) {
        auto shift = last - first + 1;
        auto suggestedIndex = impl().index - shift;
        impl().expectedIndex = suggestedIndex;
    }
}

void ListModelItemProxy::onModelReset()
{
    assert(!impl().model || impl().model->columnCount() == 1);
    reload();
}

void ListModelItemProxy::onRowsMoved(const QModelIndex& parent, int /*start*/, int /*end*/, const QModelIndex& destination, int /*row*/)
{
    assert(parent == destination);
    reload();
    // TODO: optimize
}

void ListModelItemProxy::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (!isValidIndex())
        return;

    auto first = topLeft.row();
    auto last = bottomRight.row();

    if (first > impl().index || last < impl().index)
        return;

    reloadRoles(roles);
}

void ListModelItemProxy::onLayoutAboutToBeChanged(const QList<QPersistentModelIndex>& /*parents*/, QAbstractItemModel::LayoutChangeHint /*hint*/)
{
    // Nothing.
}

void ListModelItemProxy::onLayoutChanged(const QList<QPersistentModelIndex>& /*parents*/, QAbstractItemModel::LayoutChangeHint /*hint*/)
{
    assert(!impl().model || impl().model->columnCount() == 1);
    reload();
}

void ListModelItemProxy::onColumnsAboutToBeInserted(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    assert(!"Unsupported");
}

void ListModelItemProxy::onColumnsAboutToBeRemoved(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    assert(!"Unsupported");
}

void ListModelItemProxy::onColumnsAboutToBeMoved(const QModelIndex& /*sourceParent*/, int /*sourceStart*/, int /*sourceEnd*/, const QModelIndex& /*destinationParent*/, int /*destinationColumn*/)
{
    assert(!"Unsupported");
}

void ListModelItemProxy::setPropertyMap(QQmlPropertyMap* value)
{
    if (impl().propertyMap == value)
        return;

    if (impl().propertyMap)
        delete impl().propertyMap;

    impl().propertyMap = value;
    emit propertyMapChanged(impl().propertyMap);
}

void ListModelItemProxy::setKeepIndexTrack(bool value)
{
    if (impl().keepIndexTrack == value)
        return;

    impl().keepIndexTrack = value;
    emit keepIndexTrackChanged(impl().keepIndexTrack);
}

int ListModelItemProxy::count() const
{
    return impl().model ? impl().model->rowCount() : 0;
}
