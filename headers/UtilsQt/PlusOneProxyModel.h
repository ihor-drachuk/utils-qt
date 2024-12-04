/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <QAbstractListModel>
#include <utils-cpp/default_ctor_ops.h>
#include <utils-cpp/pimpl.h>
#include <optional>

/* PlusOneProxyModel is used for appending/prepending one artificial item
   to model's content. Typical case: when you have model + view, but
   you also need to show "+" button in same list.

   Additional roles are appended: 'isArtificial' and 'artificialValue'.
   - isArtificial    -- is bool, which is true for injected rows.
   - artificialValue -- is QVariant, which is copied from 'artificialValue' property
*/

class PlusOneProxyModel : public QAbstractListModel
{
    Q_OBJECT
    NO_COPY_MOVE(PlusOneProxyModel);

public:
    enum Mode {
        Append,
        Prepend
    };
    Q_ENUM(Mode)

    Q_PROPERTY(QAbstractListModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
    Q_PROPERTY(PlusOneProxyModel::Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QVariant artificialValue READ artificialValue WRITE setArtificialValue NOTIFY artificialValueChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    explicit PlusOneProxyModel(QObject* parent = nullptr);
    ~PlusOneProxyModel() override;

    static void registerTypes();

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

// --- Properties support ---
public:
    QAbstractListModel* sourceModel() const;
    PlusOneProxyModel::Mode mode() const;
    const QVariant& artificialValue() const;
    bool enabled() const;

public slots:
    void setSourceModel(QAbstractListModel* value);
    void setMode(PlusOneProxyModel::Mode value);
    void setArtificialValue(const QVariant& value);
    void setEnabled(bool value);

signals:
    void sourceModelChanged(QAbstractListModel* sourceModel);
    void modeChanged(PlusOneProxyModel::Mode mode);
    void artificialValueChanged(const QVariant& artificialValue);
    void enabledChanged(bool enabled);
// --- ---

private:
    bool initable() const;
    void init();
    void deinit();

    void connectModel();

    int remapIndexFromSrc(int index) const;
    int remapIndexToSrc(int index) const;
    QModelIndex remapIndexFromSrc(const QModelIndex& index) const;
    QModelIndex remapIndexToSrc(const QModelIndex& index) const;
    std::optional<int> augmentedIndex(bool enforce = false) const;
    std::optional<QModelIndex> augmentedIndexQt() const;

    void onModelDestroyed();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onBeforeInserted(const QModelIndex& parent, int first, int last);
    void onAfterInserted(const QModelIndex& parent, int first, int last);
    void onBeforeRemoved(const QModelIndex& parent, int first, int last);
    void onAfterRemoved(const QModelIndex& parent, int first, int last);
    void onBeforeReset();
    void onAfterReset();
    void onBeforeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);
    void onAfterMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);

private:
    DECLARE_PIMPL
};
