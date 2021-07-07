#pragma once
#include <QAbstractListModel>
#include <QJSValue>
#include <utils-cpp/pimpl.h>

class ListModelTools : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QAbstractListModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QStringList roles READ roles /*WRITE setRoles*/ NOTIFY rolesChanged)
    Q_PROPERTY(bool allowJsValues READ allowJsValues WRITE setAllowJsValues NOTIFY allowJsValuesChanged)
    Q_PROPERTY(bool allowRoles READ allowRoles WRITE setAllowRoles NOTIFY allowRolesChanged)
    Q_PROPERTY(int itemsCount READ itemsCount /*WRITE setItemsCount*/ NOTIFY itemsCountChanged)

    static void registerTypes(const char* url);

    explicit ListModelTools(QObject* parent = nullptr);
    ~ListModelTools() override;

    Q_INVOKABLE QVariant getData(int index, const QString& role = {}) const;
    Q_INVOKABLE QVariantMap getDataByRoles(int index, const QStringList& roles) const;

signals:
    void beforeModelReset();
    void modelReset();
    void beforeInserted(int index1, int index2);
    void inserted(int index1, int index2);
    void beforeRemoved(int index1, int index2, QJSValue tester);
    void removed(int index1, int index2, QJSValue tester);
    void changed(int index1, int index2, QJSValue tester,
                 const QStringList& affectedRoles);

// --- Properties support ---
public:
    QAbstractListModel* model() const;
    bool allowJsValues() const;
    int itemsCount() const;
    bool allowRoles() const;
    QStringList roles() const;

public slots:
    void setModel(QAbstractListModel* value);
    void setAllowJsValues(bool value);
    void setItemsCount(int value);
    void setAllowRoles(bool value);
    void setRoles(QStringList value);

signals:
    void modelChanged(QAbstractListModel* model);
    void allowJsValuesChanged(bool allowJsValues);
    void itemsCountChanged(int itemsCount);
    void allowRolesChanged(bool allowRoles);
    void rolesChanged(const QStringList& roles);
// --- ---

private:
    QJSValue createTester(int low, int high);
    void updateItemsCount();
    QStringList listRoles() const;
    void fillRolesMap();

    // From model
    void onBeforeRowsInserted(const QModelIndex& parent, int first, int last);
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onBeforeRowsRemoved(const QModelIndex& parent, int first, int last);
    void onRowsRemoved(const QModelIndex& parent, int first, int last);
    void onBeforeModelReset();
    void onModelReset();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

private:
    DECLARE_PIMPL
};
