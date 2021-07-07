#include "utils-qt/qml-cpp/ListModelTools.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QMap>

struct ListModelTools::impl_t
{
    QAbstractListModel* model { nullptr };
    QStringList roles;
    QMap<QString, int> rolesMap;
    bool allowJsValues { false };
    bool allowRoles { false };
    int itemsCount { 0 };
};


void ListModelTools::registerTypes(const char* url)
{
    qRegisterMetaType<QAbstractListModel*>("QAbstractListModel*");
    qmlRegisterType<ListModelTools>(url, 1, 0, "ListModelTools");
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
        result[*it] = impl().model->data(impl().model->index(index), impl().rolesMap.value(*it, -1));
        it++;
    }

    return result;
}

QAbstractListModel* ListModelTools::model() const
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

bool ListModelTools::allowJsValues() const
{
    return impl().allowJsValues;
}

void ListModelTools::setModel(QAbstractListModel* value)
{
    if (!qobject_cast<QAbstractListModel*>(value)) {
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
        QObject::connect(impl().model, &QAbstractListModel::modelAboutToBeReset, this, &ListModelTools::onBeforeModelReset);
        QObject::connect(impl().model, &QAbstractListModel::modelReset, this, &ListModelTools::onModelReset);

        QObject::connect(impl().model, &QAbstractListModel::rowsAboutToBeInserted, this, &ListModelTools::onBeforeRowsInserted);
        QObject::connect(impl().model, &QAbstractListModel::rowsInserted, this, &ListModelTools::onRowsInserted);

        QObject::connect(impl().model, &QAbstractListModel::rowsAboutToBeRemoved, this, &ListModelTools::onBeforeRowsRemoved);
        QObject::connect(impl().model, &QAbstractListModel::rowsRemoved, this, &ListModelTools::onRowsRemoved);

        QObject::connect(impl().model, &QAbstractListModel::dataChanged, this, &ListModelTools::onDataChanged);
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
    updateItemsCount();
    emit modelReset();
}

void ListModelTools::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    auto first = topLeft.row();
    auto last = bottomRight.row();
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
