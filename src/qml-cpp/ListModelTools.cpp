#include <utils-qt/qml-cpp/ListModelTools.h>

#include <QQmlEngine>
#include <QQmlContext>
#include <QMap>
#include <QSet>
#include <QHash>

namespace {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) // Qt 5.14 (^) and upper

template <template<typename> typename Container, typename T>
QSet<T> toSet(const Container<T>& c) {
    return QSet<T>(c.cbegin(), c.cend());
}

template <template<typename> typename Container, typename T>
QVector<T> toVector(const Container<T>& c) {
    return QVector<T>(c.cbegin(), c.cend());
}

#else // Before Qt 5.14 (v)

template <typename T>
QSet<T> toSet(const QList<T>& c) {
    return c.toSet();
}

template <typename T>
QSet<T> toSet(const QVector<T>& c) {
    return c.toList().toSet();
}

template <typename T>
QVector<T> toVector(const QList<T>& c) {
    return c.toVector();
}

template <typename T>
QVector<T> toVector(const QSet<T>& c) {
    return c.toList().toVector();
}
#endif // End QT_VERSION

} // namespace


struct ListModelTools::impl_t
{
    QAbstractItemModel* model { nullptr };
    QStringList roles;
    QMap<QString, int> rolesMap;
    bool allowJsValues { false };
    bool allowRoles { false };
    int itemsCount { 0 };
    bool bufferChanges { true };
    int bufferingCnt { 0 };
    QVector<int> bufferedRoles;
    int bufferingIndex { -1 };
};


void ListModelTools::registerTypes()
{
    qRegisterMetaType<QAbstractItemModel*>("QAbstractItemModel*");
    qmlRegisterType<ListModelTools>("UtilsQt", 1, 0, "ListModelTools");
}


ListModelTools::ListModelTools(QObject* parent)
    : QObject(parent)
{
    createImpl();
}

ListModelTools::~ListModelTools()
{
}

QVariant ListModelTools::getData(int index, const QString& role) const
{
    if (role.isEmpty()) {
        return getDataByRoles(index, impl().roles);
    } else {
        auto v = getDataByRoles(index, {role});

        if (v.isEmpty())
            return v;

        return v.value(role);
    }
}

QVariantMap ListModelTools::getDataByRoles(int index, const QStringList& roles) const
{
    if (!impl().model)
        return {};

    assert(index >= 0 && index < impl().model->rowCount());
    if (!(index >= 0 && index < impl().model->rowCount())) {
        return {};
    }

    const QStringList& rolesRef = roles.isEmpty() ?
                                      impl().roles :
                                      roles;

    QVariantMap result;
    auto it = rolesRef.cbegin();
    const auto itEnd = rolesRef.cend();
    while (it != itEnd) {
        assert(impl().rolesMap.contains(*it));
        auto newValue = impl().model->data(impl().model->index(index, 0), impl().rolesMap.value(*it, -1));
        result[*it] = newValue.isValid() ? newValue : QVariant::fromValue(nullptr);
        it++;
    }

    return result;
}

void ListModelTools::setData(int index, const QVariant& value, const QString& role)
{
    if (!impl().model)
        return;

    assert(index >= 0 && index < impl().model->rowCount());
    if (!(index >= 0 && index < impl().model->rowCount())) {
        return;
    }

    assert(!role.isEmpty());
    if (!(!role.isEmpty())) {
        return;
    }

    assert(impl().rolesMap.contains(role));

    impl().model->setData(impl().model->index(index, 0), value, impl().rolesMap.value(role, -1));
}

void ListModelTools::setDataByRoles(int index, const QVariantMap& values)
{
    if (!impl().model)
        return;

    assert(index >= 0 && index < impl().model->rowCount());
    if (!(index >= 0 && index < impl().model->rowCount())) {
        return;
    }

    if (values.isEmpty())
        return;

    auto it = values.cbegin();
    const auto itEnd = values.cend();
    impl().bufferingIndex = index;
    updBufferingCnt(1);
    while (it != itEnd) {
        assert(impl().rolesMap.contains(it.key()));
        impl().model->setData(impl().model->index(index, 0), it.value(), impl().rolesMap.value(it.key(), -1));
        it++;
    }
    updBufferingCnt(-1);
}

int ListModelTools::roleNameToInt(const QString& role) const
{
    if (!impl().model)
        return -1;

    return impl().model->roleNames().key(role.toLatin1(), -1);
}

QModelIndex ListModelTools::modelIndexByRow(int row)
{
    if (!impl().model)
        return {};

    return impl().model->index(row, 0);
}

std::optional<int> ListModelTools::findIndexByValue(const QAbstractItemModel& model, const QByteArray& roleName, const QVariant& value)
{
    assert(&model);
    assert(model.columnCount() == 1);
    auto sz = model.rowCount();
    auto role = model.roleNames().key(roleName, -1);
    assert(role >= 0);

    for (int i = 0; i < sz; i++) {
        auto idx = model.index(i, 0);
        auto val = model.data(idx, role);
        if (val == value) {
            return i;
        }
    }

    return {};
}

std::optional<QVariant> ListModelTools::findValueByValues(const QAbstractItemModel& model, const QVariantMap& values, const QString& neededRole)
{
    assert(&model);
    assert(!values.isEmpty());
    assert(model.columnCount() == 1);
    auto sz = model.rowCount();

    auto modelRoles = model.roleNames();

    assert(neededRole.isEmpty() || modelRoles.values().contains(neededRole.toLatin1()));

    QHash<QString, int> rolesMap;
    { // ctx
        auto it = modelRoles.cbegin();
        const auto itEnd = modelRoles.cend();
        while (it != itEnd) {
            rolesMap.insert(QString::fromLatin1(it.value()), it.key());
            it++;
        }
    }

    QHash<int, QVariant> expectedValues;
    { // ctx
        auto it = values.cbegin();
        const auto itEnd = values.cend();
        while (it != itEnd) {
            assert(rolesMap.contains(it.key()));
            expectedValues.insert(rolesMap.value(it.key()), it.value());
            it++;
        }
    }

    for (int i = 0; i < sz; i++) {
        auto idx = model.index(i, 0);

        bool allRolesMatched = true;
        for (auto it = expectedValues.cbegin(),
             itEnd = expectedValues.cend();
             it != itEnd;
             it++)
        {
            auto data = model.data(idx, it.key());
            if (data != it.value()) {
                allRolesMatched = false;
                break;
            }
        }

        if (!allRolesMatched) continue;

        if (neededRole.isEmpty()) {
            QVariantMap result;
            for (auto it = modelRoles.cbegin(),
                 itEnd = modelRoles.cend();
                 it != itEnd;
                 it++)
            {
                result.insert(QString::fromLatin1(it.value()), model.data(idx, it.key()));
            }

            return result;

        } else {
            return model.data(idx, rolesMap.value(neededRole));
        }
    }

    return {};
}

QVariantList ListModelTools::collectValuesByRole(const QAbstractItemModel& model, const QByteArray& roleName)
{
    assert(&model);
    assert(model.columnCount() == 1);
    auto sz = model.rowCount();
    auto role = model.roleNames().key(roleName, -1);
    assert(role >= 0);

    QVariantList result;
    result.reserve(sz);
    for (int i = 0; i < sz; i++) {
        auto idx = model.index(i, 0);
        auto value = model.data(idx, role);
        result.append(value);
    }

    return result;
}

QAbstractItemModel* ListModelTools::model() const
{
    return impl().model;
}

int ListModelTools::itemsCount() const
{
    return impl().itemsCount;
}

bool ListModelTools::allowRoles() const
{
    return impl().allowRoles;
}

QStringList ListModelTools::roles() const
{
    return impl().roles;
}

bool ListModelTools::bufferChanges() const
{
    return impl().bufferChanges;
}

bool ListModelTools::allowJsValues() const
{
    return impl().allowJsValues;
}

void ListModelTools::setModel(QAbstractItemModel* value)
{
    assert(!value || value->columnCount() == 1);

    if (!qobject_cast<QAbstractItemModel*>(value)) {
        value = nullptr;
    }

    if (impl().model == value)
        return;

    if (impl().model) {
        QObject::disconnect(impl().model, nullptr, this, nullptr);
    }

    impl().model = value;
    impl().itemsCount = impl().model ? impl().model->rowCount() : 0;
    fillRolesMap();

    if (impl().model) {
        QObject::connect(impl().model, &QAbstractItemModel::modelAboutToBeReset, this, &ListModelTools::onBeforeModelReset);
        QObject::connect(impl().model, &QAbstractItemModel::modelReset, this, &ListModelTools::onModelReset);

        QObject::connect(impl().model, &QAbstractItemModel::rowsAboutToBeInserted, this, &ListModelTools::onBeforeRowsInserted);
        QObject::connect(impl().model, &QAbstractItemModel::rowsInserted, this, &ListModelTools::onRowsInserted);

        QObject::connect(impl().model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ListModelTools::onBeforeRowsRemoved);
        QObject::connect(impl().model, &QAbstractItemModel::rowsRemoved, this, &ListModelTools::onRowsRemoved);

        QObject::connect(impl().model, &QAbstractItemModel::dataChanged, this, &ListModelTools::onDataChanged);
    }

    emit modelChanged(impl().model);
    emit itemsCountChanged(impl().itemsCount);
    emit rolesChanged(impl().roles);
}

void ListModelTools::setItemsCount(int value)
{
    if (impl().itemsCount == value)
        return;

    impl().itemsCount = value;
    emit itemsCountChanged(impl().itemsCount);
}

void ListModelTools::setAllowRoles(bool value)
{
    if (impl().allowRoles == value)
        return;

    impl().allowRoles = value;
    emit allowRolesChanged(impl().allowRoles);
}

void ListModelTools::setRoles(QStringList value)
{
    if (impl().roles == value)
        return;

    impl().roles = value;
    emit rolesChanged(impl().roles);
}

void ListModelTools::setBufferChanges(bool value)
{
    if (impl().bufferChanges == value)
        return;

    impl().bufferChanges = value;
    emit bufferChangesChanged(impl().bufferChanges);
}

void ListModelTools::setAllowJsValues(bool value)
{
    if (impl().allowJsValues == value)
        return;

    impl().allowJsValues = value;
    emit allowJsValuesChanged(impl().allowJsValues);
}

QJSValue ListModelTools::createTester(int low, int high)
{
    if (impl().allowJsValues) {
        auto ctx = QQmlEngine::contextForObject(this);
        auto engine = ctx->engine();
        engine->evaluate(QString("( function(exports) {"
            "exports.tester = function(index) { return (index >= %1) && (index <= %2); };"
        "})(this.listModelTools = {})").arg(low).arg(high));
        auto f = engine->globalObject().property("listModelTools").property("tester");
        return f;
    } else {
        return QJSValue(QJSValue::SpecialValue::NullValue);
    }
}

void ListModelTools::updateItemsCount()
{
    setItemsCount(impl().model ? impl().model->rowCount() : 0);
}

QStringList ListModelTools::listRoles() const
{
    if (!impl().model)
        return {};

    QStringList result;
    auto roles = impl().model->roleNames();
    for (const auto& x : qAsConst(roles)) {
        result.append(QString::fromLatin1(x));
    }

    return result;
}

void ListModelTools::fillRolesMap()
{
    impl().rolesMap.clear();
    impl().roles.clear();

    if (!impl().model)
        return;

    auto roles = impl().model->roleNames();
    auto it = roles.cbegin();
    const auto itEnd = roles.cend();
    while (it != itEnd) {
        auto value = QString::fromLatin1(it.value());
        impl().rolesMap.insert(value, it.key());
        impl().roles.append(value);
        it++;
    }
}

void ListModelTools::updBufferingCnt(int delta)
{
    if (!impl().bufferChanges)
        return;

    impl().bufferingCnt += delta;
    assert(impl().bufferingCnt >= 0);

    if (impl().bufferingCnt == 0) {
        auto idx = impl().model->index(impl().bufferingIndex, 0);
        auto roles = toSet(impl().bufferedRoles);
        impl().bufferedRoles.clear();

        onDataChanged(idx, idx, toVector(roles));
    }
}

void ListModelTools::onBeforeRowsInserted(const QModelIndex& /*parent*/, int first, int last)
{
    emit beforeInserted(first, last);
}

void ListModelTools::onRowsInserted(const QModelIndex& /*parent*/, int first, int last)
{
    updateItemsCount();
    emit inserted(first, last);
}

void ListModelTools::onBeforeRowsRemoved(const QModelIndex& /*parent*/, int first, int last)
{
    emit beforeRemoved(first, last, createTester(first, last));
}

void ListModelTools::onRowsRemoved(const QModelIndex& /*parent*/, int first, int last)
{
    updateItemsCount();
    emit removed(first, last, createTester(first, last));
}

void ListModelTools::onBeforeModelReset()
{
    emit beforeModelReset();
}

void ListModelTools::onModelReset()
{
    fillRolesMap();
    updateItemsCount();
    emit modelReset();
}

void ListModelTools::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    auto first = topLeft.row();
    auto last = bottomRight.row();

    if (impl().bufferingCnt && first == impl().bufferingIndex && last == impl().bufferingIndex) {
        impl().bufferedRoles.append(roles);
        return;
    }

    auto tester = createTester(first, last);
    QStringList roleNames;

    if (impl().allowRoles) {
        auto modelRoles = impl().model->roleNames();

        if (roles.isEmpty()) {
            for (const auto& x : qAsConst(modelRoles)) {
                roleNames.append(QString::fromLatin1(x));
            }
        } else {
            auto it = modelRoles.begin();
            const auto itEnd = modelRoles.end();
            while (it != itEnd) {
                if (roles.contains(it.key()))
                    roleNames.append(QString::fromLatin1(it.value()));
                it++;
            }
        }
    }

    emit changed(first, last, tester, roleNames);
}
