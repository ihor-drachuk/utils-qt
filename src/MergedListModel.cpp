/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/MergedListModel.h>

#include <QRegularExpression>
#include <QDataStream>
#include <QBuffer>
#include <QQmlEngine>
#include <QSet>
#include <cassert>
#include <optional>
#include <unordered_map>
#include <utils-cpp/scoped_guard.h>
#include <utils-cpp/container_utils.h>
#include <UtilsQt/convertcontainer.h>
#include <UtilsQt/Qml-Cpp/QmlUtils.h>

namespace {

std::optional<int> getInt(const QVariant& value)
{
    switch (value.type()) {
        case QVariant::Type::Int:
        case QVariant::Type::UInt:
        case QVariant::Type::LongLong:
        case QVariant::Type::ULongLong: {
            bool ok;
            auto result = value.toInt(&ok);
            assert(ok);
            return result;
        }

        default:
            return {};
    }
}

std::optional<QString> getString(const QVariant& value)
{
    switch (value.type()) {
        case QVariant::Type::String:
            return value.toString();

        case QVariant::Type::ByteArray:
            return QString::fromLatin1(value.toByteArray());

        default:
            return {};
    }
}


struct ModelContext
{
    QAbstractListModel* model { nullptr };
    int joinRole { -1 };
    bool operationInProgress { false };
    std::unordered_map<int, int> roleRemapFromSrc; // Src role -> local role idx
    std::unordered_map<int, int> roleRemapToSrc;   // Local role idx -> src role
    std::unordered_map<int, int> indexRemapFromSrc;
    std::unordered_map<int, int> indexRemapToSrc;

    void reset() {
        joinRole = -1;
        operationInProgress = false;
        roleRemapFromSrc.clear();
        roleRemapToSrc.clear();
        indexRemapFromSrc.clear();
        indexRemapToSrc.clear();
    }
};
} // namespace

namespace std {
template<>
struct hash<QVariant>
{
    std::size_t operator()(const QVariant& k) const
    {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream temp(&buffer);
        temp << k;
        return qHash(buffer.buffer());
    }
};
} // namespace std

#ifdef NDEBUG // If Release
#define NeedSelfCheck
#else
//#define NeedSelfCheck auto _selfCheck = CreateScopedGuard([this](){ selfCheck(); });
#define NeedSelfCheck
#endif

#ifdef NDEBUG // If Release
#define mlm_assert_rel(x) if (!(x)) std::abort()
#else
#define mlm_assert_rel(x) assert(x)
#endif

struct MergedListModel::impl_t
{
    QVariant joinRole1;
    QVariant joinRole2;
    QMap<QVariant, Converter> providedResetters;

    // Cache
    ModelContext models[2];

    QList<QByteArray> roles;
    int joinRole {-1};
    int srcRole {-1};
    QList<QVariantList> data;
    std::unordered_map<QVariant, int> joinValueToIndex;

    std::unordered_map<int, Converter> resetters;

    // Flags
    bool isInitialized { false };
    bool resetting { false };

    //
    void reset() {
        isInitialized = false;
        resetting = false;

        roles.clear();
        joinRole = -1;
        srcRole = -1;
        data.clear();
        joinValueToIndex.clear();
        resetters.clear();

        models[0].reset();
        models[1].reset();
    }
};


MergedListModel::MergedListModel(QObject* parent)
    : QAbstractListModel(parent)
{
    createImpl();
}

MergedListModel::~MergedListModel()
{
}

void MergedListModel::registerTypes()
{
    qmlRegisterType<MergedListModel>("UtilsQt", 1, 0, "MergedListModel");
}

int MergedListModel::rowCount(const QModelIndex&) const
{
    return impl().isInitialized ? impl().data.size() : 0;
}

QVariant MergedListModel::data(const QModelIndex& index, int role) const
{
    if (!impl().isInitialized)
        return {};

    if (!index.isValid())
        return {};

    role -= Qt::UserRole;
    auto idx = index.row();

    assert(role >= 0 && role < impl().roles.size());
    assert(idx >= 0 && idx < impl().data.size());

    return impl().data.at(idx).at(role);
}

bool MergedListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!impl().isInitialized)
        return {};

    if (!index.isValid())
        return {};

    assert(!impl().models[0].operationInProgress);
    assert(!impl().models[1].operationInProgress);

    role -= Qt::UserRole;
    auto idx = index.row();

    assert(role >= 0 && role < impl().roles.size());
    assert(idx >= 0 && idx < impl().data.size());

    if (role == impl().joinRole) {
        auto srcNum = impl().data.at(idx).at(impl().srcRole).toInt();
        assert(srcNum == 3);

        // Don't allow to modify joinRoles

        return false;

    } else {
        auto foundInModel1 = utils_cpp::find_in_map(impl().models[0].roleRemapToSrc, role);
        auto foundInModel2 = utils_cpp::find_in_map(impl().models[1].roleRemapToSrc, role);
        assert(foundInModel1.has_value() ^ foundInModel2.has_value());
        auto modelIdx = foundInModel1 ? 0 : 1;
        auto& ctx = impl().models[modelIdx];

        auto remappedIdx = utils_cpp::find_in_map(ctx.indexRemapToSrc, idx);
        if (remappedIdx) {
            ctx.model->setData(ctx.model->index(*remappedIdx), value, ctx.roleRemapToSrc.at(role));
            return true;
        } else {
            // Nothing. There is no such role in source model for this row.
            return false;
        }
    }
}

QHash<int, QByteArray> MergedListModel::roleNames() const
{
    if (!impl().isInitialized)
        return {};

    QHash<int, QByteArray> result;

    for (int i = 0; i < impl().roles.size(); i++)
        result.insert(Qt::UserRole + i, impl().roles.at(i));

    return result;
}

void MergedListModel::checkConsistency() const
{
    selfCheck();
}

void MergedListModel::registerCustomResetter(const QVariant& role, const Converter& converter)
{
    assert(utils_cpp::find_in_map(impl().providedResetters, role).has_value() == false);
    auto it = impl().providedResetters.insert(role, converter);

    if (impl().isInitialized)
        addResetterToCache(it);
}

void MergedListModel::registerCustomResetter(const Converter& converter)
{
    registerCustomResetter(-1, converter);
}

QAbstractListModel* MergedListModel::model1() const
{
    return impl().models[0].model;
}

QAbstractListModel* MergedListModel::model2() const
{
    return impl().models[1].model;
}

QVariant MergedListModel::joinRole1() const
{
    return impl().joinRole1;
}

QVariant MergedListModel::joinRole2() const
{
    return impl().joinRole2;
}

void MergedListModel::setModel1(QAbstractListModel* value)
{
    if (impl().models[0].model == value)
        return;

    if (impl().models[0].model)
        QObject::disconnect(impl().models[0].model, nullptr, this, nullptr);

    impl().models[0].model = value;
    init();

    emit model1Changed(impl().models[0].model);
}

void MergedListModel::setModel2(QAbstractListModel* value)
{
    if (impl().models[1].model == value)
        return;

    if (impl().models[1].model)
        QObject::disconnect(impl().models[1].model, nullptr, this, nullptr);

    impl().models[1].model = value;
    init();

    emit model2Changed(impl().models[1].model);
}

void MergedListModel::setJoinRole1(const QVariant& value)
{
    if (impl().joinRole1 == value)
        return;

    impl().joinRole1 = value;
    init();

    emit joinRole1Changed(impl().joinRole1);
}

void MergedListModel::setJoinRole2(const QVariant& value)
{
    if (impl().joinRole2 == value)
        return;

    impl().joinRole2 = value;
    init();

    emit joinRole2Changed(impl().joinRole2);
}

void MergedListModel::selfCheckModel(int idx) const
{
    const auto& ctx = impl().models[idx];
    auto rowsCnt = rowCount({});
    auto ctxRowsCnt = ctx.model->rowCount({});

    { // ctx
        QSet<int> indexes, indexes2;

        mlm_assert_rel(ctx.indexRemapFromSrc.size() == ctx.indexRemapToSrc.size());

        for (auto it = ctx.indexRemapFromSrc.cbegin(),
             itEnd = ctx.indexRemapFromSrc.cend();
             it != itEnd;
             it++)
        {
            mlm_assert_rel(it->first >= 0);
            mlm_assert_rel(it->first < ctxRowsCnt);

            mlm_assert_rel(it->second >= 0);
            mlm_assert_rel(it->second < rowsCnt);

            mlm_assert_rel(!indexes.contains(it->first));
            indexes.insert(it->first);

            mlm_assert_rel(!indexes2.contains(it->second));
            indexes2.insert(it->second);

            auto remapped = ctx.indexRemapToSrc.at(it->second);
            mlm_assert_rel(remapped == it->first);
        }
    }

    { // ctx
        mlm_assert_rel(ctx.roleRemapFromSrc.size() == ctx.roleRemapToSrc.size());
        auto rolesSz = impl().roles.size();

        for (auto it = ctx.roleRemapFromSrc.cbegin(),
             itEnd = ctx.roleRemapFromSrc.cend();
             it != itEnd;
             it++)
        {
            mlm_assert_rel(it->second >= 0);
            mlm_assert_rel(it->second < rolesSz);
        }
    }

    { // ctx
        for (auto it = ctx.roleRemapFromSrc.cbegin(),
             itEnd = ctx.roleRemapFromSrc.cend();
             it != itEnd;
             it++)
        {
            auto remapped = ctx.roleRemapToSrc.at(it->second);
            mlm_assert_rel(remapped == it->first);
        }
    }

    { // ctx
        for (int i = 0; i < ctxRowsCnt; i++) {
            auto remapped = utils_cpp::find_in_map(ctx.indexRemapFromSrc, i);
            mlm_assert_rel(remapped.has_value());
        }
    }
}

void MergedListModel::selfCheck() const
{
    // Check indexes consistency
    auto rowsCnt = rowCount({});
    mlm_assert_rel(rowsCnt == impl().data.size());

    { // ctx
        QSet<int> indexes;

        for (auto it = impl().joinValueToIndex.cbegin(),
             itEnd = impl().joinValueToIndex.cend();
             it != itEnd;
             it++)
        {
            mlm_assert_rel(it->second >= 0);
            mlm_assert_rel(it->second < rowsCnt);
            mlm_assert_rel(!indexes.contains(it->second));
            indexes.insert(it->second);
        }
    }

    { // ctx
        for (int i = 0; i < rowsCnt; i++) {
            auto hasRemap1 = utils_cpp::find_in_map(impl().models[0].indexRemapToSrc, i);
            auto hasRemap2 = utils_cpp::find_in_map(impl().models[1].indexRemapToSrc, i);
            mlm_assert_rel(hasRemap1 || hasRemap2);
        }
    }

    // Check models
    selfCheckModel(0);
    selfCheckModel(1);

    // Check data
    for (int i = 0; i < rowsCnt; i++) {
        const auto& currentLine = impl().data.at(i);

        for (int r = 0; r < impl().roles.size(); r++) {
            const QVariant value = currentLine.at(r);

            if (r == impl().srcRole) {
                mlm_assert_rel(value.type() == QVariant::Type::Int ||
                       value.type() == QVariant::Type::UInt ||
                       value.type() == QVariant::Type::LongLong ||
                       value.type() == QVariant::Type::ULongLong);

                auto intValue = value.toInt();
                mlm_assert_rel(intValue == 1 || intValue == 2 || intValue == 3);

                auto remappedIndex1 = utils_cpp::find_in_map(impl().models[0].indexRemapToSrc, i);
                auto remappedIndex2 = utils_cpp::find_in_map(impl().models[1].indexRemapToSrc, i);

                switch (intValue) {
                    case 1: {
                        mlm_assert_rel(remappedIndex1);
                        mlm_assert_rel(!remappedIndex2);
                        break;
                    }

                    case 2: {
                        mlm_assert_rel(!remappedIndex1);
                        mlm_assert_rel(remappedIndex2);
                        break;
                    }

                    case 3: {
                        mlm_assert_rel(remappedIndex1);
                        mlm_assert_rel(remappedIndex2);
                        break;
                    }
                }

            } else {
                auto remappedIndex1 = utils_cpp::find_in_map(impl().models[0].indexRemapToSrc, i);
                auto remappedIndex2 = utils_cpp::find_in_map(impl().models[1].indexRemapToSrc, i);
                auto remappedRole1 = utils_cpp::find_in_map(impl().models[0].roleRemapToSrc, r);
                auto remappedRole2 = utils_cpp::find_in_map(impl().models[1].roleRemapToSrc, r);

                mlm_assert_rel(remappedIndex1 || remappedIndex2);
                mlm_assert_rel(remappedRole1 || remappedRole2);

                if (remappedIndex1 && remappedRole1) {
                    auto srcValue = impl().models[0].model->data(impl().models[0].model->index(remappedIndex1.value()), remappedRole1.value());
                    mlm_assert_rel(value == srcValue);
                }

                if (remappedIndex2 && remappedRole2) {
                    auto srcValue = impl().models[1].model->data(impl().models[1].model->index(remappedIndex2.value()), remappedRole2.value());
                    mlm_assert_rel(value == srcValue);
                }

                if (r == impl().joinRole) {
                    auto remappedJoinValue = utils_cpp::find_in_map(impl().joinValueToIndex, value);
                    mlm_assert_rel(remappedJoinValue);
                    mlm_assert_rel(remappedJoinValue.value() == i);
                }
            }
        }
    }


    // Reconstruction test
#if 0
    {
        MergedListModel clone;
        clone.setModel1(impl().models[0].model);
        clone.setModel2(impl().models[1].model);
        clone.setJoinRole1(impl().joinRole1);
        clone.setJoinRole2(impl().joinRole2);
        mlm_assert_rel(impl().roles == clone._impl->roles);
        mlm_assert_rel(impl().data == clone._impl->data);
        mlm_assert_rel(impl().joinValueToIndex == clone._impl->joinValueToIndex);

        mlm_assert_rel(impl().models[0].roleRemapFromSrc == clone._impl->models[0].roleRemapFromSrc);
        mlm_assert_rel(impl().models[0].roleRemapToSrc == clone._impl->models[0].roleRemapToSrc);
        mlm_assert_rel(impl().models[0].indexRemapFromSrc == clone._impl->models[0].indexRemapFromSrc);
        mlm_assert_rel(impl().models[0].indexRemapToSrc == clone._impl->models[0].indexRemapToSrc);

        mlm_assert_rel(impl().models[1].roleRemapFromSrc == clone._impl->models[1].roleRemapFromSrc);
        mlm_assert_rel(impl().models[1].roleRemapToSrc == clone._impl->models[1].roleRemapToSrc);
        mlm_assert_rel(impl().models[1].indexRemapFromSrc == clone._impl->models[1].indexRemapFromSrc);
        mlm_assert_rel(impl().models[1].indexRemapToSrc == clone._impl->models[1].indexRemapToSrc);
    }
#endif
}

bool MergedListModel::initable() const
{
    return impl().models[0].model && impl().models[1].model && impl().joinRole1.isValid() && impl().joinRole2.isValid();
}

void MergedListModel::init()
{
    if (impl().resetting)
        return;

    deinit();
    if (!initable()) return;

    beginResetModel();
    auto _endResetModel = CreateScopedGuard([this](){ endResetModel(); });

    connectModel(0);
    connectModel(1);

    // Find join roles
    int role1 {-1};
    int role2 {-1};
    if (auto intValue = getInt(impl().joinRole1)) {
        if (impl().models[0].model->roleNames().contains(*intValue))
            role1 = *intValue;
    } else if (auto strValue = getString(impl().joinRole1)) {
        auto matchingIds = impl().models[0].model->roleNames().keys(strValue->toLatin1());
        if (matchingIds.size() == 1)
            role1 = matchingIds.first();
    }

    if (auto intValue = getInt(impl().joinRole2)) {
        if (impl().models[1].model->roleNames().contains(*intValue))
            role2 = *intValue;
    } else if (auto strValue = getString(impl().joinRole2)) {
        auto matchingIds = impl().models[1].model->roleNames().keys(strValue->toLatin1());
        if (matchingIds.size() == 1)
            role2 = matchingIds.first();
    }

    if (role1 == -1 || role2 == -1)
        return;

    impl().models[0].joinRole = role1;
    impl().models[1].joinRole = role2;

    // Fill own roles and own join role
    int i = 0;
    auto roleNames0 = impl().models[0].model->roleNames();
    for (auto it = roleNames0.cbegin(),
         itEnd = roleNames0.cend();
         it != itEnd;
         it++, i++)
    {
        impl().roles.append(it.value());
        impl().models[0].roleRemapFromSrc.insert({it.key(), i});
        impl().models[0].roleRemapToSrc.insert({i, it.key()});

        if (it.key() == role1)
            impl().joinRole = i;
    }

    auto roleNames1 = impl().models[1].model->roleNames();
    for (auto it = roleNames1.cbegin(),
         itEnd = roleNames1.cend();
         it != itEnd;
         it++)
    {
        if (it.key() == impl().models[1].joinRole) {
            impl().models[1].roleRemapFromSrc.insert({it.key(), impl().joinRole});
            impl().models[1].roleRemapToSrc.insert({impl().joinRole, it.key()});
        } else {
            impl().roles.append(it.value());
            impl().models[1].roleRemapFromSrc.insert({it.key(), i});
            impl().models[1].roleRemapToSrc.insert({i, it.key()});
            i++;
        }
    }

    // Set srcRole
    impl().srcRole = impl().roles.size();
    impl().roles.append("source");

    // Fill data from 1st model
    auto count = impl().models[0].model->rowCount();
    for (int i = 0; i < count; i++) {
        QVariantList line;

        for (int r = 0; r < impl().roles.size(); r++) {
            QVariant value;

            if (r == impl().srcRole) {
                // 'source' role
                value = 1;
            } else {
                // If role exists in this model
                if (impl().models[0].roleRemapToSrc.count(r)) {
                    value = impl().models[0].model->data(impl().models[0].model->index(i), impl().models[0].roleRemapToSrc.at(r));
                }
            }

            line.append(value);

            // Add 'join' role to map (fast search: joinValue -> line index)
            if (r == impl().joinRole && !QmlUtils::instance().isNull(value)) {
                assert(utils_cpp::find_in_map(impl().joinValueToIndex, value).has_value() == false);
                impl().joinValueToIndex.insert({value, impl().data.size()});
            }
        }

        impl().data.append(line);
        impl().models[0].indexRemapFromSrc.insert({i, i});
        impl().models[0].indexRemapToSrc.insert({i, i});
    }

    // Augment by data from 2nd model
    count = impl().models[1].model->rowCount();
    for (int i = 0; i < count; i++) {
        auto joinValue = impl().models[1].model->data(impl().models[1].model->index(i), impl().models[1].joinRole);
        auto foundIndexIt = impl().joinValueToIndex.find(joinValue);
        if (foundIndexIt != impl().joinValueToIndex.end()) {
            // Found. Augment line
            int foundIndex = foundIndexIt->second;
            QVariantList& currentLine = impl().data[foundIndex];

            for (int r = 0; r < impl().roles.size(); r++) {
                QVariant& currentValue = currentLine[r];

                if (r == impl().srcRole) {
                    // 'source' role
                    currentValue = currentValue.toInt() | 2;

                } else if (r == impl().joinRole) {
                    // Join role
                    // It must exist in right model
                    // We already matched join value, so right value must be equal to local/left join value
                    assert(impl().models[1].roleRemapToSrc.count(r));
                    assert(currentValue == impl().models[1].model->data(impl().models[1].model->index(i), impl().models[1].roleRemapToSrc.at(r)));

                } else {
                    // If role exists in this model
                    if (impl().models[1].roleRemapToSrc.count(r)) {
                        currentValue = impl().models[1].model->data(impl().models[1].model->index(i), impl().models[1].roleRemapToSrc.at(r));
                    }
                }
            }

            impl().models[1].indexRemapFromSrc.insert({i, foundIndex});
            impl().models[1].indexRemapToSrc.insert({foundIndex, i});

        } else {
            // Not found. Append new line
            QVariantList line;

            for (int r = 0; r < impl().roles.size(); r++) {
                QVariant value;

                if (r == impl().srcRole) {
                    // 'source' role
                    value = 2;
                } else {
                    // If role exists in this model
                    if (impl().models[1].roleRemapToSrc.count(r)) {
                        value = impl().models[1].model->data(impl().models[1].model->index(i), impl().models[1].roleRemapToSrc.at(r));
                    }
                }

                line.append(value);

                // Add 'join' role to map (fast search: joinValue -> line index)
                if (r == impl().joinRole && !QmlUtils::instance().isNull(value)) {
                    assert(utils_cpp::find_in_map(impl().joinValueToIndex, value).has_value() == false);
                    impl().joinValueToIndex.insert({value, impl().data.size()});
                }
            }

            impl().models[1].indexRemapFromSrc.insert({i, impl().data.size()});
            impl().models[1].indexRemapToSrc.insert({impl().data.size(), i});
            impl().data.append(line);
        }
    }

    // Make sure role names are unique
    QRegularExpression isAppliedFix("_mlm\\d+$");
    QRegularExpression appliedNumber("\\d+$");

    auto updateName = [&](QByteArray& roleName) {
        if (isAppliedFix.match(roleName).hasMatch()) {
            auto appliedNumberMatch = appliedNumber.match(roleName);
            assert(appliedNumberMatch.hasMatch());
            auto numberStr = appliedNumberMatch.captured();
            roleName.resize(roleName.size() - numberStr.size());
            roleName += QString::number(numberStr.toInt()+1).toLatin1();

        } else {
            roleName += "_mlm1";
        }
    };

    bool needTest = true;
    while (needTest) {
        needTest = false;

        for (int i = 1; i < impl().roles.size(); i++) {
            for (int k = 0; k < i; k++) {
                while (impl().roles.at(i) == impl().roles.at(k)) {
                    needTest = true;
                    updateName(impl().roles[i]);
                }
            }
        }
    }

    // Load resetters
    for (auto it = impl().providedResetters.cbegin(); it != impl().providedResetters.cend(); it++)
        addResetterToCache(it);

    impl().isInitialized = true;
}

void MergedListModel::deinit()
{
    beginResetModel();

    if (impl().models[0].model)
        QObject::disconnect(impl().models[0].model, nullptr, this, nullptr);

    if (impl().models[1].model)
        QObject::disconnect(impl().models[1].model, nullptr, this, nullptr);

    impl().reset();

    endResetModel();
}

void MergedListModel::resetValue(int index, int role)
{
    std::optional<Converter> resetConverter;

    if (auto resetConv = utils_cpp::find_in_map_cref(impl().resetters, role)) {
        resetConverter = *resetConv;
    } else if (auto globalResetConv = utils_cpp::find_in_map_cref(impl().resetters, -1)) {
        resetConverter = *globalResetConv;
    }

    QVariant newValue = resetConverter ?
                            resetConverter.value()(role, QLatin1String(impl().roles.at(role)), index, impl().data[index][role]) :
                            QVariant::fromValue(nullptr);

    impl().data[index][role] = newValue;
}

template<typename Iter>
void MergedListModel::addResetterToCache(Iter it)
{
    const auto providedRole = it.key();
    int role;

    if (const auto intRole = getInt(providedRole)) {
        if (*intRole == -1) {
            role = -1;
        } else {
            role = *intRole - Qt::UserRole;
            assert(role >= 0 && role < impl().roles.size() && "Failed to find resetter's role! (out-of-range)");
        }

    } else if (const auto strRole = getString(providedRole)) {
        const auto matchedRole = utils_cpp::find_cref(impl().roles, strRole->toLatin1());
        assert(matchedRole.has_value() && "Failed to find resetter's role! (wrong name)");
        role = static_cast<int>(matchedRole.index());

    } else {
        assert(false && "Wrong resetter's role type! It should be int or string!");
    }

    assert(utils_cpp::find_in_map_cref(impl().resetters, role).has_value() == false && "Same resetter's role added twice!");
    impl().resetters.insert({role, it.value()});
}

void MergedListModel::connectModel(int idx)
{
    using namespace std::placeholders;

    assert(idx == 0 || idx == 1);

    QObject::disconnect(impl().models[idx].model, nullptr, this, nullptr);
    QObject::connect(impl().models[idx].model, &QAbstractListModel::destroyed,            this, std::bind(&MergedListModel::onModelDestroyed, std::ref(*this), idx));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::dataChanged,          this, std::bind(&MergedListModel::onDataChanged, std::ref(*this), idx, _1, _2, _3));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::rowsAboutToBeInserted,this, std::bind(&MergedListModel::onBeforeInserted, std::ref(*this), idx, _1, _2, _3));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::rowsInserted,         this, std::bind(&MergedListModel::onAfterInserted, std::ref(*this), idx, _1, _2, _3));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::rowsAboutToBeRemoved, this, std::bind(&MergedListModel::onBeforeRemoved, std::ref(*this), idx, _1, _2, _3));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::rowsRemoved,          this, std::bind(&MergedListModel::onAfterRemoved, std::ref(*this), idx, _1, _2, _3));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::modelAboutToBeReset,  this, std::bind(&MergedListModel::onBeforeReset, std::ref(*this), idx));
    QObject::connect(impl().models[idx].model, &QAbstractListModel::modelReset,           this, std::bind(&MergedListModel::onAfterReset, std::ref(*this), idx));
}

void MergedListModel::onModelDestroyed(int idx)
{
    if (idx == 0) {
        setModel1(nullptr);
    } else {
        setModel2(nullptr);
    }
}

void MergedListModel::onDataChanged(int idx, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (!impl().isInitialized || impl().resetting) return;

    assert(!impl().models[0].operationInProgress);
    assert(!impl().models[1].operationInProgress);
    impl().models[idx].operationInProgress = true;

    NeedSelfCheck;

    auto& ctx = impl().models[idx];
    auto rolesFull = roles.isEmpty() ? UtilsQt::toVector(ctx.model->roleNames().keys()) : roles;

    // Lambda: update 1 line
    auto updateLine = [this, &ctx, &rolesFull, idx](int srcIndex){
        auto localIdx = ctx.indexRemapFromSrc.at(srcIndex);
        auto oldJoinValue = impl().data.at(localIdx).at(impl().joinRole);
        auto newJoinValue = ctx.model->data(ctx.model->index(srcIndex), ctx.joinRole);
        if (oldJoinValue == newJoinValue) {
            // Something changed, but joinValue still the same
            QVector<int> changedRoles;
            for (auto r : rolesFull) {
                if (r == ctx.joinRole) continue; // -- because already handled
                auto localRole = ctx.roleRemapFromSrc.at(r);
                auto newValue = ctx.model->data(ctx.model->index(srcIndex), r);
                auto oldValue = impl().data.at(localIdx).at(localRole);
                if (newValue != oldValue) {
                    impl().data[localIdx][localRole] = newValue;
                    changedRoles.append(localRole + Qt::UserRole);
                }
            }

            if (!changedRoles.isEmpty()) {
                emit dataChanged(index(localIdx), index(localIdx), changedRoles);
            }

        } else {
            // joinValue changed
            bool lineExists = true;

            if (impl().data.at(localIdx).at(impl().srcRole).toInt() == 3) {
                // Detach
                auto ctxRoles = ctx.model->roleNames().keys();
                ctxRoles.removeOne(ctx.joinRole);

                QVector<int> changedRoles;

                for (auto r : ctxRoles) {
                    auto localRole = ctx.roleRemapFromSrc.at(r);
                    resetValue(localIdx, localRole);
                    changedRoles.append(localRole + Qt::UserRole);
                }

                ctx.indexRemapToSrc.erase(localIdx);
                ctx.indexRemapFromSrc.erase(srcIndex);

                // Also update srcRole
                impl().data[localIdx][impl().srcRole] = (!idx)+1;
                changedRoles.append(impl().srcRole + Qt::UserRole);

                if (!changedRoles.isEmpty())
                    emit dataChanged(index(localIdx), index(localIdx), changedRoles);

                lineExists = false;
            } else {
                assert(impl().data.at(localIdx).at(impl().srcRole).toInt() == (idx+1));
                lineExists = true;
            }

            if (utils_cpp::find_in_map(impl().joinValueToIndex, newJoinValue)) {
                // There is joinValue same like newJoinValue

                if (lineExists) {
                    // Remove line
                    beginRemoveRows({}, localIdx, localIdx);

                    impl().joinValueToIndex.erase(oldJoinValue);
                    ctx.indexRemapToSrc.erase(localIdx);
                    ctx.indexRemapFromSrc.erase(srcIndex);
                    impl().data.removeAt(localIdx);

                    // Shift indexes
                    decltype(ctx.indexRemapFromSrc)   copyIndexRemapFromSrc;
                    decltype(ctx.indexRemapToSrc)     copyIndexRemapToSrc;
                    decltype(impl().joinValueToIndex) copyJoinValueToIndex;

                    for (const auto& x : ctx.indexRemapFromSrc) {
                        int newSrcIndex = x.first;
                        int newLocalIndex = x.second > localIdx ? x.second - 1 : x.second;

                        copyIndexRemapFromSrc.insert({newSrcIndex, newLocalIndex});
                        copyIndexRemapToSrc.insert({newLocalIndex, newSrcIndex});
                    }

                    for (const auto& x : impl().joinValueToIndex) {
                        const auto newKey = x.first;
                        const auto newValue = x.second > localIdx ? x.second - 1 : x.second;
                        copyJoinValueToIndex.insert({newKey, newValue});
                    }

                    //...don't forget to update indexes in 'another' model
                    auto& ctx2 = impl().models[!idx];
                    decltype(ctx2.indexRemapFromSrc)   copyIndexRemapFromSrc2;
                    decltype(ctx2.indexRemapToSrc)     copyIndexRemapToSrc2;

                    for (const auto& x : ctx2.indexRemapFromSrc) {
                        int newSrcIndex = x.first;
                        int newLocalIndex = x.second > localIdx ? x.second - 1 : x.second;

                        copyIndexRemapFromSrc2.insert({newSrcIndex, newLocalIndex});
                        copyIndexRemapToSrc2.insert({newLocalIndex, newSrcIndex});
                    }

                    localIdx = -1; // Invalidate localIdx after removal

                    ctx.indexRemapFromSrc = std::move(copyIndexRemapFromSrc);
                    ctx.indexRemapToSrc = std::move(copyIndexRemapToSrc);
                    ctx2.indexRemapFromSrc = std::move(copyIndexRemapFromSrc2);
                    ctx2.indexRemapToSrc = std::move(copyIndexRemapToSrc2);
                    impl().joinValueToIndex = std::move(copyJoinValueToIndex);

                    endRemoveRows();
                }

                // Attach line
                auto newLine = utils_cpp::find_in_map(impl().joinValueToIndex, newJoinValue).value();
                auto ctxRoles = ctx.model->roleNames().keys();
                ctxRoles.removeOne(ctx.joinRole);

                QVector<int> changedRoles;

                for (auto r : ctxRoles) {
                    auto localRole = ctx.roleRemapFromSrc.at(r);
                    impl().data[newLine][localRole] = ctx.model->data(ctx.model->index(srcIndex), r);
                    changedRoles.append(localRole + Qt::UserRole);
                }

                // Also update srcRole
                impl().data[newLine][impl().srcRole] = 3;
                changedRoles.append(impl().srcRole + Qt::UserRole);

                assert(utils_cpp::find_in_map(impl().joinValueToIndex, newJoinValue).has_value());
                ctx.indexRemapFromSrc.insert({srcIndex, newLine});
                ctx.indexRemapToSrc.insert({newLine, srcIndex});

                if (!changedRoles.isEmpty())
                    emit dataChanged(index(newLine), index(newLine), changedRoles);

            } else {
                if (lineExists) {
                    // Change existing line + joinRole
                    auto ctxRoles = ctx.model->roleNames().keys();
                    QVector<int> changedRoles;

                    for (auto r : ctxRoles) {
                        auto localRole = ctx.roleRemapFromSrc.at(r);
                        auto newValue = ctx.model->data(ctx.model->index(srcIndex), r);
                        auto& currentValue = impl().data[localIdx][localRole];

                        if (newValue != currentValue) {
                            if (r == ctx.joinRole) {
                                impl().joinValueToIndex.erase(currentValue);

                                if (!QmlUtils::instance().isNull(newValue))
                                    impl().joinValueToIndex.insert({newValue, localIdx});
                            }

                            currentValue = newValue;
                            changedRoles.append(localRole + Qt::UserRole);
                        }
                    }

                    if (!changedRoles.isEmpty())
                        emit dataChanged(index(localIdx), index(localIdx), changedRoles);
                } else {
                    // Add new line (data, srcRole, indexes, signals)
                    auto newLocalIndex = impl().data.size();

                    beginInsertRows({}, newLocalIndex, newLocalIndex);

                    QVariantList line;

                    for (int r = 0; r < impl().roles.size(); r++) {
                        QVariant value;

                        if (r == impl().srcRole) {
                            // 'source' role
                            value = idx+1;
                        } else {
                            // If role exists in this model
                            if (ctx.roleRemapToSrc.count(r)) {
                                value = ctx.model->data(ctx.model->index(srcIndex), ctx.roleRemapToSrc.at(r));
                            }
                        }

                        line.append(value);

                        // Add 'join' role to map
                        if (r == impl().joinRole && !QmlUtils::instance().isNull(value)) {
                            assert(utils_cpp::find_in_map(impl().joinValueToIndex, value).has_value() == false);
                            impl().joinValueToIndex.insert({value, newLocalIndex});
                        }
                    }

                    impl().data.append(line);
                    ctx.indexRemapFromSrc.insert({srcIndex, newLocalIndex});
                    ctx.indexRemapToSrc.insert({newLocalIndex, srcIndex});

                    endInsertRows();
                }
            }
        }
    };


    if (!roles.isEmpty() && !roles.contains(ctx.joinRole)) {
        // Optimized solution for cases, when joinRole is not affected
        int minIndex = -1;
        int maxIndex = -1;
        bool rangeInit = false;

        for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
            auto localIdx = ctx.indexRemapFromSrc.at(i);

            if (rangeInit) {
                minIndex = std::min(minIndex, localIdx);
                maxIndex = std::max(maxIndex, localIdx);
            } else {
                minIndex = localIdx;
                maxIndex = localIdx;
                rangeInit = true;
            }

            for (auto r : rolesFull) {
                assert(r != ctx.joinRole);
                auto localRole = ctx.roleRemapFromSrc.at(r);
                auto newValue = ctx.model->data(ctx.model->index(i), r);
                auto oldValue = impl().data.at(localIdx).at(localRole);
                if (newValue != oldValue) {
                    impl().data[localIdx][localRole] = newValue;
                }
            }
        }

        assert(rangeInit);

        QVector<int> localRoles;
        localRoles.reserve(roles.size());
        for (const auto& x : roles) localRoles.append(ctx.roleRemapFromSrc.at(x) + Qt::UserRole);

        emit dataChanged(index(minIndex), index(maxIndex), localRoles);

    } else {
        // General solution
        // Loop: update all lines
        for (int i = topLeft.row(); i <= bottomRight.row(); i++)
            updateLine(i);
    }

    impl().models[idx].operationInProgress = false;
}

void MergedListModel::onBeforeInserted(int idx, const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    if (!impl().isInitialized || impl().resetting) return;
    assert(!impl().models[0].operationInProgress);
    assert(!impl().models[1].operationInProgress);
    impl().models[idx].operationInProgress = true;
}

void MergedListModel::onAfterInserted(int idx, const QModelIndex& /*parent*/, int first, int last)
{
    if (!impl().isInitialized || impl().resetting) return;

    assert(impl().models[idx].operationInProgress == true);
    assert(impl().models[!idx].operationInProgress == false);
    auto& ctx = impl().models[idx];

    NeedSelfCheck;

    // Shift all indexes
    decltype(ctx.indexRemapFromSrc) copyIndexRemapFromSrc;
    decltype(ctx.indexRemapToSrc)   copyIndexRemapToSrc;
    int delta = last - first + 1;
    for (const auto& x : ctx.indexRemapFromSrc) {
        auto newSrcIndex = (x.first >= first) ? x.first + delta : x.first;
        auto newLocalIndex = x.second;

        copyIndexRemapFromSrc.insert({newSrcIndex, newLocalIndex});
        copyIndexRemapToSrc.insert({newLocalIndex, newSrcIndex});
    }

    ctx.indexRemapFromSrc = std::move(copyIndexRemapFromSrc);
    ctx.indexRemapToSrc = std::move(copyIndexRemapToSrc);

    // Handle src-inserted items
    for (int i = first; i <= last; i++) {
        // There is new line
        // Decide: should we insert it or update existing
        auto joinValue = ctx.model->data(ctx.model->index(i), ctx.joinRole);
        auto updatingLineIdx = utils_cpp::find_in_map(impl().joinValueToIndex, joinValue);
        if (updatingLineIdx) {
            // Update existing
            QVector<int> updatedRoles;
            QVariantList& currentLine = impl().data[*updatingLineIdx];

            for (auto r = 0; r < impl().roles.size(); r++) {
                QVariant& currentValue = currentLine[r];

                if (r == impl().srcRole) {
                    currentValue = 3;
                    updatedRoles.append(r + Qt::UserRole);

                } else if (r == impl().joinRole) {
                    auto srcRole = utils_cpp::find_in_map(ctx.roleRemapToSrc, r);
                    assert(srcRole.has_value());
                    auto newValue = ctx.model->data(ctx.model->index(i), *srcRole);
                    assert(currentValue == newValue);

                } else {
                    auto srcRole = utils_cpp::find_in_map(ctx.roleRemapToSrc, r);
                    if (srcRole) {
                        auto newValue = ctx.model->data(ctx.model->index(i), *srcRole);
                        if (currentValue != newValue) {
                            currentValue = newValue;
                            updatedRoles.append(r + Qt::UserRole);
                        }
                    }
                }
            }

            // Update indexes & append
            assert(utils_cpp::find_in_map(ctx.indexRemapToSrc, *updatingLineIdx).has_value() == false);
            assert(utils_cpp::find_in_map(ctx.indexRemapFromSrc, i).has_value() == false);
            ctx.indexRemapFromSrc.insert({i, *updatingLineIdx});
            ctx.indexRemapToSrc.insert({*updatingLineIdx, i});

            // Notify
            if (!updatedRoles.empty())
                emit dataChanged(index(*updatingLineIdx), index(*updatingLineIdx), updatedRoles);

        } else {
            // Append new line
            QVariantList line;

            for (auto r = 0; r < impl().roles.size(); r++) {
                QVariant currentValue;

                if (r == impl().srcRole) {
                    currentValue = (idx == 0) ? 1 : 2;
                } else {
                    auto srcRole = utils_cpp::find_in_map(ctx.roleRemapToSrc, r);
                    if (srcRole)
                        currentValue = ctx.model->data(ctx.model->index(i), *srcRole);
                }

                line.append(currentValue);
            }

            // Update indexes & append
            auto newIndex = impl().data.size();

            beginInsertRows({}, newIndex, newIndex);

            auto joinValue = line.at(impl().joinRole);
            assert(utils_cpp::find_in_map(impl().joinValueToIndex, joinValue).has_value() == false);
            assert(utils_cpp::find_in_map(ctx.indexRemapFromSrc, i).has_value() == false);
            assert(utils_cpp::find_in_map(ctx.indexRemapToSrc,   newIndex).has_value() == false);
            if (!QmlUtils::instance().isNull(joinValue))
                impl().joinValueToIndex.insert({joinValue, newIndex});
            ctx.indexRemapFromSrc.insert({i, newIndex});
            ctx.indexRemapToSrc.insert({newIndex, i});

            impl().data.append(line);

            endInsertRows();
        }
    }

    // Reset 'busy' flag
    impl().models[idx].operationInProgress = false;
}

void MergedListModel::onBeforeRemoved(int idx, const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    if (!impl().isInitialized || impl().resetting) return;
    assert(!impl().models[0].operationInProgress);
    assert(!impl().models[1].operationInProgress);
    impl().models[idx].operationInProgress = true;
}

void MergedListModel::onAfterRemoved(int idx, const QModelIndex& /*parent*/, int first, int last)
{
    if (!impl().isInitialized || impl().resetting) return;

    assert(impl().models[idx].operationInProgress == true);
    assert(impl().models[!idx].operationInProgress == false);
    auto& ctx = impl().models[idx];

    NeedSelfCheck;

    // Handle src-removed items
    for (int i = first; i <= last; i++) {
        auto localIdx = ctx.indexRemapFromSrc.at(i);
        auto srcRoleValue = impl().data.at(localIdx).at(impl().srcRole).toInt();
        assert(srcRoleValue == (idx+1) || srcRoleValue == 3);
        auto foundInBoth = (srcRoleValue == 3);
        bool removedLocal = false;
        std::optional<scoped_guard<std::function<void()>>> endRemoveRowsLater;
        std::optional<scoped_guard<std::function<void()>>> dataChangedLater;

        if (foundInBoth) {
            // Update line
            QVector<int> updatedRoles;

            for (int r = 0; r < impl().roles.size(); r++) {
                if (r == impl().srcRole) {
                    impl().data[localIdx][r] = (!idx) + 1;
                    updatedRoles.append(r);

                } else if (r == impl().joinRole) {
                    // Nothing.

                } else if (utils_cpp::find_in_map(ctx.roleRemapToSrc, r)) {
                    resetValue(localIdx, r);
                    updatedRoles.append(r);
                }
            }

            // Notify
            if (!updatedRoles.isEmpty()) {
                for (auto& x : updatedRoles)
                    x += Qt::UserRole;

                // emit dataChanged(index(localIdx), index(localIdx), updatedRoles);
                dataChangedLater = scoped_guard<std::function<void()>>([this, localIdx, updatedRoles](){ emit dataChanged(index(localIdx), index(localIdx), updatedRoles); });
            }

        } else {
            beginRemoveRows({}, localIdx, localIdx);

            // Update indexes
            impl().joinValueToIndex.erase(impl().data.at(localIdx).at(impl().joinRole));
            removedLocal = true;

            // Remove line
            impl().data.removeAt(localIdx);

            //endRemoveRows();
            endRemoveRowsLater = scoped_guard<std::function<void()>>([this](){ endRemoveRows(); });
        }

        // Remove indexes
        ctx.indexRemapFromSrc.erase(i);
        ctx.indexRemapToSrc.erase(localIdx);

        // Shift all indexes
        decltype(ctx.indexRemapFromSrc) copyIndexRemapFromSrc;
        decltype(ctx.indexRemapToSrc)   copyIndexRemapToSrc;

        for (const auto& x : ctx.indexRemapFromSrc) {
            int newSrcIndex = x.first > i ? x.first - 1 : x.first;
            int newLocalIndex = removedLocal && (x.second > localIdx) ? x.second - 1 : x.second;

            copyIndexRemapFromSrc.insert({newSrcIndex, newLocalIndex});
            copyIndexRemapToSrc.insert({newLocalIndex, newSrcIndex});
        }

        ctx.indexRemapFromSrc = std::move(copyIndexRemapFromSrc);
        ctx.indexRemapToSrc = std::move(copyIndexRemapToSrc);

        if (removedLocal) {
            decltype(impl().joinValueToIndex) copyJoinValueToIndex;

            for (const auto& x : impl().joinValueToIndex) {
                const auto newKey = x.first;
                const auto newValue = x.second > localIdx ? x.second - 1 : x.second;
                copyJoinValueToIndex.insert({newKey, newValue});
            }

            impl().joinValueToIndex = std::move(copyJoinValueToIndex);

            //...don't forget to update indexes in 'another' model context
            auto& ctx2 = impl().models[!idx];
            decltype(ctx2.indexRemapFromSrc)   copyIndexRemapFromSrc2;
            decltype(ctx2.indexRemapToSrc)     copyIndexRemapToSrc2;

            for (const auto& x : ctx2.indexRemapFromSrc) {
                int newSrcIndex = x.first;
                int newLocalIndex = x.second > localIdx ? x.second - 1 : x.second;

                copyIndexRemapFromSrc2.insert({newSrcIndex, newLocalIndex});
                copyIndexRemapToSrc2.insert({newLocalIndex, newSrcIndex});
            }

            ctx2.indexRemapFromSrc = std::move(copyIndexRemapFromSrc2);
            ctx2.indexRemapToSrc = std::move(copyIndexRemapToSrc2);
        }
    }

    // Reset 'busy' flag
    impl().models[idx].operationInProgress = false;
}

void MergedListModel::onBeforeReset(int idx)
{
    if (!impl().isInitialized) return;
    assert(!impl().resetting);
    assert(!impl().models[0].operationInProgress);
    assert(!impl().models[1].operationInProgress);
    impl().models[idx].operationInProgress = true;
    impl().resetting = true;
}

void MergedListModel::onAfterReset(int idx)
{
    impl().resetting = false;
    assert(impl().models[idx].operationInProgress == true);
    assert(impl().models[!idx].operationInProgress == false);
    NeedSelfCheck;
    init();
    impl().models[idx].operationInProgress = false;
}
