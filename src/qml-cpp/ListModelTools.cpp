#include "utils-qt/qml-cpp/ListModelTools.h"

#include <QQmlEngine>


struct ListModelTools::impl_t
{
    QAbstractListModel* model { nullptr };
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

QAbstractListModel* ListModelTools::model() const
{
    return impl().model;
}

int ListModelTools::itemsCount() const
{
    return impl().itemsCount;
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
    emit modelChanged(impl().model);

    if (impl().model) {
        QObject::connect(impl().model, &QAbstractListModel::rowsInserted, this, &ListModelTools::onRowsInserted);
        QObject::connect(impl().model, &QAbstractListModel::rowsRemoved, this, &ListModelTools::onRowsRemoved);
        QObject::connect(impl().model, &QAbstractListModel::modelReset, this, &ListModelTools::onModelReset);
    }

    updateItemsCount();
}

void ListModelTools::setItemsCount(int value)
{
    if (impl().itemsCount == value)
        return;

    impl().itemsCount = value;
    emit itemsCountChanged(impl().itemsCount);
}

void ListModelTools::updateItemsCount()
{
    setItemsCount(impl().model ? impl().model->rowCount() : 0);
}

void ListModelTools::onRowsInserted(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    updateItemsCount();
}

void ListModelTools::onRowsRemoved(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    updateItemsCount();
}

void ListModelTools::onModelReset()
{
    updateItemsCount();
}
