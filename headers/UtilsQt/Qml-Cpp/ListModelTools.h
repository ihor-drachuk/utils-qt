/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <optional>
#include <QAbstractItemModel>
#include <QVector>
#include <QVariantList>
#include <QJSValue>
#include <utils-cpp/default_ctor_ops.h>
#include <utils-cpp/pimpl.h>

class ListModelTools : public QObject
{
    Q_OBJECT
    NO_COPY_MOVE(ListModelTools);
public:
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QStringList roles READ roles /*WRITE setRoles*/ NOTIFY rolesChanged)
    Q_PROPERTY(bool allowJsValues READ allowJsValues WRITE setAllowJsValues NOTIFY allowJsValuesChanged)
    Q_PROPERTY(bool allowRoles READ allowRoles WRITE setAllowRoles NOTIFY allowRolesChanged)
    Q_PROPERTY(int itemsCount READ itemsCount /*WRITE setItemsCount*/ NOTIFY itemsCountChanged)
    Q_PROPERTY(bool bufferChanges READ bufferChanges WRITE setBufferChanges NOTIFY bufferChangesChanged)

    static void registerTypes();

    explicit ListModelTools(QObject* parent = nullptr);
    ~ListModelTools() override;

    Q_INVOKABLE QVariant getData(int index, const QString& role = {}) const;
    Q_INVOKABLE QVariantMap getDataByRoles(int index, const QStringList& roles = {}) const;
    Q_INVOKABLE void setData(int index, const QVariant& value, const QString& role = {});
    Q_INVOKABLE void setDataByRoles(int index, const QVariantMap& values);

    Q_INVOKABLE QVariantList collectData(const QString& role = {}, int firstIndex = -1, int lastIndex = -1) const;
    Q_INVOKABLE QVariantList collectDataByRoles(const QStringList& roles = {}, int firstIndex = -1, int lastIndex = -1) const;

    Q_INVOKABLE int roleNameToInt(const QString& role) const;
    Q_INVOKABLE QModelIndex modelIndexByRow(int row);

    static std::optional<int> findIndexByValue(
            const QAbstractItemModel& model,
            const QByteArray& roleName,
            const QVariant& value);

    static std::optional<QVariant> findValueByValues(
            const QAbstractItemModel& model,
            const QVariantMap& values,
            const QString& neededRole = {});

    static QVariantList collectValuesByRole(const QAbstractItemModel& model,
            const QByteArray& roleName);

    template<typename T>
    static QVector<T> collectValuesByRoleT(
            const QAbstractItemModel& model,
            const QByteArray& roleName)
    {
        auto values = collectValuesByRole(model, roleName);
        QVector<T> result;
        result.reserve(values.size());
        for (const auto& x : std::as_const(values))
            result.append(x.value<T>());
        return result;
    }

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
    QAbstractItemModel* model() const;
    bool allowJsValues() const;
    int itemsCount() const;
    bool allowRoles() const;
    QStringList roles() const;
    bool bufferChanges() const;

public slots:
    void setModel(QAbstractItemModel* value);
    void setAllowJsValues(bool value);
    void setItemsCount(int value);
    void setAllowRoles(bool value);
    void setRoles(QStringList value);
    void setBufferChanges(bool value);

signals:
    void modelChanged(QAbstractItemModel* model);
    void allowJsValuesChanged(bool allowJsValues);
    void itemsCountChanged(int itemsCount);
    void allowRolesChanged(bool allowRoles);
    void rolesChanged(const QStringList& roles);
    void bufferChangesChanged(bool bufferChanges);
// --- ---

private:
    QJSValue createTester(int low, int high);
    void updateItemsCount();
    QStringList listRoles() const;
    void fillRolesMap();
    void updBufferingCnt(int delta);

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
