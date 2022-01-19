#pragma once
#include <QAbstractItemModel>
#include <QMap>
#include <QVariant>

#include <utils-cpp/pimpl.h>
#include <functional>
#include <variant>


//
// Extends model with additional roles
//

class ExtendedModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    using RoleDataGetHandler = std::function<QVariant(int /*row*/, int /*column*/, const QMap<QByteArray, QVariant>& /*rolesData*/, const QVariant& /*prevValue*/)>;
    using RoleDataSetHandler = std::function<QMap<QByteArray, QVariant>(int /*row*/, int /*column*/, const QVariant& /*newValue*/, const QVariant& /*prevValue*/)>;

    struct DependsOnAllOther {};
    struct DependsNo {};
    struct DependsOn { DependsOn(const QList<QByteArray>& deps): deps(deps) {}; QList<QByteArray> deps; };
    using RoleDependencies = std::variant<DependsOnAllOther, DependsNo, DependsOn>;

public:
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)

    explicit ExtendedModel(QObject* parent = nullptr);
    ~ExtendedModel() override;

    static void registerTypes();

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    void addRoleHandler(const QByteArray& role,
                        const RoleDependencies& dependencies,
                        const RoleDataGetHandler& getHandler,
                        const RoleDataSetHandler& setHandler,
                        bool rebuildModel = true);

// --- Properties support ---
public:
    QAbstractItemModel* sourceModel() const;

public slots:
    void setSourceModel(QAbstractItemModel* value);

signals:
    void sourceModelChanged(QAbstractItemModel* sourceModel);
// --- ---

private: // Source model handlers
    // ...

private:
    void rebuild();

private:
    DECLARE_PIMPL
};
