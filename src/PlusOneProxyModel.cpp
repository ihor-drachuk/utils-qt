/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/PlusOneProxyModel.h>

#include <cassert>
#include <QQmlEngine>
#include <QList>

namespace {

const char* const isArtificialRole = "isArtificial";
const char* const artificialValueRole = "artificialValue";

} // namespace

struct PlusOneProxyModel::impl_t
{
    QAbstractListModel* sourceModel {};
    PlusOneProxyModel::Mode mode {PlusOneProxyModel::Mode::Append};
    QVariant artificialValue;
    bool enabled {true};

    bool isInitialized {false};
    int roleIsArtificial {-1};
    int roleArtificialValue {-1};
    bool cascaded {false};
    QList<QMetaObject::Connection> modelConnections;

    void reset() {
        isInitialized = false;
        roleIsArtificial = -1;
        roleArtificialValue = -1;

        for (auto& x : modelConnections)
            QObject::disconnect(x);

        modelConnections.clear();
    }
};


PlusOneProxyModel::PlusOneProxyModel(QObject* parent)
    : QAbstractListModel(parent)
{
    createImpl();
}

PlusOneProxyModel::~PlusOneProxyModel()
{
}

void PlusOneProxyModel::registerTypes()
{
    qmlRegisterType<PlusOneProxyModel>("UtilsQt", 1, 0, "PlusOneProxyModel");
}

int PlusOneProxyModel::rowCount(const QModelIndex& /*parent*/) const
{
    if (!impl().isInitialized)
        return 0;

    return impl().sourceModel->rowCount({}) + (impl().enabled ? 1 : 0);
}

QVariant PlusOneProxyModel::data(const QModelIndex& index, int role) const
{
    if (!impl().isInitialized)
        return {};

    if (auto aIdx = augmentedIndex(); aIdx && index.row() == *aIdx) {
        if (role == impl().roleIsArtificial) {
            return true;

        } else if (role == impl().roleArtificialValue) {
            return impl().artificialValue;

        } else {
            return QVariant::fromValue(nullptr);
        }

    } else {
        if (impl().cascaded) {
            return impl().sourceModel->data(remapIndexToSrc(index), role);

        } else {
            if (role == impl().roleIsArtificial) {
                return false;

            } else if (role == impl().roleArtificialValue) {
                return QVariant::fromValue(nullptr);

            } else {
                return impl().sourceModel->data(remapIndexToSrc(index), role);
            }
        }
    }
}

bool PlusOneProxyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!impl().isInitialized)
        return false;

    if (auto aIdx = augmentedIndex(); aIdx && index.row() == *aIdx) {
        assert(false && "Attempt to write to read-only row!");
        return false;

    } else {
        if (role == impl().roleIsArtificial || role == impl().roleArtificialValue) {
            assert(false && "Attempt to write to read-only role!");
            return false;

        } else {
            return impl().sourceModel->setData(remapIndexToSrc(index), value, role);
        }
    }
}

QHash<int, QByteArray> PlusOneProxyModel::roleNames() const
{
    if (!impl().isInitialized)
        return {};

    auto result = impl().sourceModel->roleNames();

    if (!impl().cascaded) {
        assert(impl().roleIsArtificial > 0 &&
               impl().roleArtificialValue > 0);

        result.insert(impl().roleIsArtificial,    isArtificialRole);
        result.insert(impl().roleArtificialValue, artificialValueRole);
    }

    return result;
}

QAbstractListModel* PlusOneProxyModel::sourceModel() const
{
    return impl().sourceModel;
}

void PlusOneProxyModel::setSourceModel(QAbstractListModel* value)
{
    if (impl().sourceModel == value)
        return;

    deinit();
    impl().sourceModel = value;
    init();

    emit sourceModelChanged(impl().sourceModel);
}

PlusOneProxyModel::Mode PlusOneProxyModel::mode() const
{
    return impl().mode;
}

void PlusOneProxyModel::setMode(PlusOneProxyModel::Mode value)
{
    if (impl().mode == value)
        return;

    if (auto aIdx = augmentedIndex()) {
        beginRemoveRows({}, *aIdx, *aIdx);
        endRemoveRows();

        impl().mode = value;

        aIdx = augmentedIndex();
        assert(aIdx);
        beginInsertRows({}, *aIdx, *aIdx);
        endInsertRows();

    } else {
        impl().mode = value;
    }

    emit modeChanged(impl().mode);
}

const QVariant& PlusOneProxyModel::artificialValue() const
{
    return impl().artificialValue;
}

void PlusOneProxyModel::setArtificialValue(const QVariant& value)
{
    if (impl().artificialValue == value)
        return;

    impl().artificialValue = value;

    if (auto idx = augmentedIndexQt())
        emit dataChanged(*idx, *idx, {impl().roleArtificialValue});

    emit artificialValueChanged(impl().artificialValue);
}

bool PlusOneProxyModel::enabled() const
{
    return impl().enabled;
}

void PlusOneProxyModel::setEnabled(bool value)
{
    if (impl().enabled == value)
        return;

    if (impl().isInitialized) {
        const auto aIdx = augmentedIndex(true);
        assert(aIdx);

        if (value) {
            beginInsertRows({}, *aIdx, *aIdx);
            impl().enabled = value;
            endInsertRows();

        } else {
            beginRemoveRows({}, *aIdx, *aIdx);
            impl().enabled = value;
            endRemoveRows();
        }

    } else {
        impl().enabled = value;
    }

    emit enabledChanged(impl().enabled);
}

bool PlusOneProxyModel::initable() const
{
    return impl().sourceModel;
}

void PlusOneProxyModel::init()
{
    deinit();

    if (!initable())
        return;

    // Determine roles and 'cascaded' status
    const auto srcRoles = impl().sourceModel->roleNames();
    if (srcRoles.values().contains(isArtificialRole)) {
        assert(srcRoles.values().contains(artificialValueRole));
        impl().roleIsArtificial    = srcRoles.key(isArtificialRole);
        impl().roleArtificialValue = srcRoles.key(artificialValueRole);
        impl().cascaded = true;

    } else {
        auto roles = srcRoles.keys();
        roles.append(Qt::UserRole);
        const auto maxUsedRole = *std::max_element(roles.cbegin(), roles.cend());
        impl().roleIsArtificial = maxUsedRole + 1;
        impl().roleArtificialValue = maxUsedRole + 2;
    }

    connectModel();
    impl().isInitialized = true;

    beginResetModel();
    endResetModel();
}

void PlusOneProxyModel::deinit()
{
    if (!impl().isInitialized)
        return;

    beginResetModel();
    impl().reset();
    endResetModel();
}

void PlusOneProxyModel::connectModel()
{
    using namespace std::placeholders;
    const auto Reg = [this](const auto& x){ impl().modelConnections.append(x); };
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::destroyed,            this, std::bind(&PlusOneProxyModel::onModelDestroyed, std::ref(*this))));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::dataChanged,          this, std::bind(&PlusOneProxyModel::onDataChanged, std::ref(*this), _1, _2, _3)));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::rowsAboutToBeInserted,this, std::bind(&PlusOneProxyModel::onBeforeInserted, std::ref(*this), _1, _2, _3)));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::rowsInserted,         this, std::bind(&PlusOneProxyModel::onAfterInserted, std::ref(*this), _1, _2, _3)));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::rowsAboutToBeRemoved, this, std::bind(&PlusOneProxyModel::onBeforeRemoved, std::ref(*this), _1, _2, _3)));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::rowsRemoved,          this, std::bind(&PlusOneProxyModel::onAfterRemoved, std::ref(*this), _1, _2, _3)));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::modelAboutToBeReset,  this, std::bind(&PlusOneProxyModel::onBeforeReset, std::ref(*this))));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::modelReset,           this, std::bind(&PlusOneProxyModel::onAfterReset, std::ref(*this))));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::rowsAboutToBeMoved,   this, std::bind(&PlusOneProxyModel::onBeforeMoved, std::ref(*this), _1, _2, _3, _4, _5)));
    Reg(QObject::connect(impl().sourceModel, &QAbstractListModel::rowsMoved,            this, std::bind(&PlusOneProxyModel::onAfterMoved, std::ref(*this), _1, _2, _3, _4, _5)));
}

int PlusOneProxyModel::remapIndexFromSrc(int index) const
{
    assert(impl().isInitialized);
    int result = (impl().enabled && impl().mode == Mode::Prepend) ? index + 1 : index;
    return result;
}

int PlusOneProxyModel::remapIndexToSrc(int index) const
{
    assert(impl().isInitialized);
    assert(!augmentedIndex() || index != *augmentedIndex());
    int result = (impl().enabled && impl().mode == Mode::Prepend) ? index - 1 : index;
    return result;
}

QModelIndex PlusOneProxyModel::remapIndexFromSrc(const QModelIndex& index) const
{
    assert(impl().isInitialized);
    assert(index.column() == 0);
    return this->index(remapIndexFromSrc(index.row()), index.column());
}

QModelIndex PlusOneProxyModel::remapIndexToSrc(const QModelIndex& index) const
{
    assert(impl().isInitialized);
    assert(index.column() == 0);
    return impl().sourceModel->index(remapIndexToSrc(index.row()), index.column());
}

std::optional<int> PlusOneProxyModel::augmentedIndex(bool enforce) const
{
    if (impl().isInitialized && (impl().enabled || enforce)) {
        return impl().mode == Mode::Prepend ? 0 : impl().sourceModel->rowCount({});
    } else {
        return {};
    }
}

std::optional<QModelIndex> PlusOneProxyModel::augmentedIndexQt() const
{
    if (auto row = augmentedIndex()) {
        return index(*row);
    } else {
        return {};
    }
}

void PlusOneProxyModel::onModelDestroyed()
{
    setSourceModel(nullptr);
}

void PlusOneProxyModel::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    assert(impl().isInitialized);

    const auto newTL = remapIndexFromSrc(topLeft);
    const auto newBR = remapIndexFromSrc(bottomRight);
    emit dataChanged(newTL, newBR, roles);
}

void PlusOneProxyModel::onBeforeInserted(const QModelIndex& /*parent*/, int first, int last)
{
    assert(impl().isInitialized);
    beginInsertRows({}, remapIndexFromSrc(first), remapIndexFromSrc(last));
}

void PlusOneProxyModel::onAfterInserted(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    assert(impl().isInitialized);
    endInsertRows();
}

void PlusOneProxyModel::onBeforeRemoved(const QModelIndex& /*parent*/, int first, int last)
{
    assert(impl().isInitialized);
    beginRemoveRows({}, remapIndexFromSrc(first), remapIndexFromSrc(last));
}

void PlusOneProxyModel::onAfterRemoved(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
    assert(impl().isInitialized);
    endRemoveRows();
}

void PlusOneProxyModel::onBeforeReset()
{
    // Nothing.
}

void PlusOneProxyModel::onAfterReset()
{
    init();
}

void PlusOneProxyModel::onBeforeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    assert(impl().isInitialized);
    assert(sourceParent == destinationParent);
    beginMoveRows({}, remapIndexFromSrc(sourceStart), remapIndexFromSrc(sourceEnd), {}, remapIndexFromSrc(destinationRow));
}

void PlusOneProxyModel::onAfterMoved(const QModelIndex& parent, int /*start*/, int /*end*/, const QModelIndex& destination, int /*row*/)
{
    assert(impl().isInitialized);
    assert(parent == destination);
    endMoveRows();
}
