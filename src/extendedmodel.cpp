#include <utils-qt/extendedmodel.h>

#define RegisterInQml 0

#include <cassert>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <utils-cpp/scoped_guard.h>

#if RegisterInQml == 1
    #include <QQmlEngine>
#endif


#ifdef NDEBUG // If Release
    #ifdef assert
        #undef assert
    #endif

    #define assert(x) if (!(x)) std::abort()
#endif


namespace {

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct RoleInfo
{
    int roleInt { -1 };
    QByteArray role;
    ExtendedModel::RoleDependencies dependencies;
    ExtendedModel::RoleDataGetHandler getHandler;
    ExtendedModel::RoleDataSetHandler setHandler;
};

} // namespace

struct ExtendedModel::impl_t
{
    QAbstractItemModel* sourceModel { nullptr };

    QHash<int, QByteArray> sourceRoles;
    QList<RoleInfo> userRoles;
    QHash<int, QByteArray> combinedRoles;
    QHash<int, int> userRolesMapping;     // Public -> Local index
    QMultiHash<int, int> dependenciesMap; // Public -> Local index
};


ExtendedModel::ExtendedModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    createImpl();
}

ExtendedModel::~ExtendedModel()
{
}

void ExtendedModel::registerTypes()
{
#if RegisterInQml == 1
    qmlRegisterType<ExtendedModel>("UtilsQt", 1, 0, "ExtendedModel");
#endif
}

int ExtendedModel::rowCount(const QModelIndex& /*parent*/) const
{
    return impl().sourceModel ? impl().sourceModel->rowCount({}) : 0;
}

QVariant ExtendedModel::data(const QModelIndex& index, int role) const
{

}

bool ExtendedModel::setData(const QModelIndex& index, const QVariant& value, int role)
{

}

QHash<int, QByteArray> ExtendedModel::roleNames() const
{
    return impl().combinedRoles;
}

void ExtendedModel::addRoleHandler(const QByteArray& role,
                                   const RoleDependencies& dependencies,
                                   const RoleDataGetHandler& getHandler,
                                   const RoleDataSetHandler& setHandler,
                                   bool rebuildModel)
{
    impl().userRoles.append({-1, role, dependencies, getHandler, setHandler});

    if (rebuildModel)
        rebuild();
}

QAbstractItemModel* ExtendedModel::sourceModel() const
{
    return impl().sourceModel;
}

void ExtendedModel::setSourceModel(QAbstractItemModel* value)
{
    if (impl().sourceModel == value)
        return;
    impl().sourceModel = value;
    rebuild();
    emit sourceModelChanged(impl().sourceModel);
}

void ExtendedModel::rebuild()
{
    beginResetModel();
    auto _endResetModel = CreateScopedGuard([this](){ endResetModel(); });

    impl().sourceRoles.clear();
    impl().combinedRoles.clear();
    impl().userRolesMapping.clear();
    impl().dependenciesMap.clear();

    if (!impl().sourceModel)
        return;

    // Source roles
    impl().sourceRoles = impl().sourceModel->roleNames();

    // Minimal role
    int minimalRoleNum = Qt::UserRole;

    for (auto it = impl().sourceRoles.cbegin(),
         itEnd = impl().sourceRoles.cend();
         it != itEnd;
         it++)
    {
        if (it.key() > minimalRoleNum)
            minimalRoleNum = it.key() + 1;
    }

    // User roles
    for (auto& x : impl().userRoles)
        x.roleInt = minimalRoleNum++;

    // Combined roles
    impl().combinedRoles = impl().sourceRoles;

    for (const auto& x : qAsConst(impl().userRoles))
        impl().combinedRoles.insert(x.roleInt, x.role);

    // Roles dependencies
    for (int i = 0; i < impl().userRoles.size(); i++) {
        const auto& userRole = impl().userRoles.at(i);

        std::visit(overloaded {
            [this, i](ExtendedModel::DependsOnAllOther) {
                for (auto it = impl().combinedRoles.cbegin(),
                     itEnd = impl().combinedRoles.cend();
                     it != itEnd;
                     it++)
                {
                    if (it.key() == i)
                        continue;

                    impl().dependenciesMap.insert(it.key(), i);
                }
            },

            [](ExtendedModel::DependsNo) {
            },

            [this, i](const ExtendedModel::DependsOn& deps) {
                for (const auto& x : deps.deps) {
                    // User role 'i' depends on role 'x'.
                    auto depRoleInt = impl().combinedRoles.key(x);
                    impl().dependenciesMap.insert(depRoleInt, i);
                }
            }
        }, userRole.dependencies);
    }

    // User role mapping
    for (int i = 0; i < impl().userRoles.size(); i++) {
        impl().userRolesMapping.insert(impl().userRoles.at(i).roleInt, i);
    }

    // Calculate data for all user roles
    // ...
}
