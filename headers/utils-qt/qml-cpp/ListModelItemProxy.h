#pragma once
#include <QObject>
#include <QQmlPropertyMap>
#include <QAbstractListModel>
#include <utils-cpp/pimpl.h>

class ListModelItemProxy : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QAbstractListModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int count READ count /*WRITE setCount*/ NOTIFY countChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(bool keepIndexTrack READ keepIndexTrack WRITE setKeepIndexTrack NOTIFY keepIndexTrackChanged)

    Q_PROPERTY(QQmlPropertyMap* propertyMap READ propertyMap /*WRITE setPropertyMap*/ NOTIFY propertyMapChanged)
    Q_PROPERTY(bool ready READ ready /*WRITE setReady*/ NOTIFY readyChanged)

    static void registerTypes();

    explicit ListModelItemProxy(QObject* parent = nullptr);
    ~ListModelItemProxy() override;

signals:
    void changed();
    void removed();
    void suggestedNewIndex(int oldIndex, int newIndex);

// --- Properties support ---
public:
    QAbstractListModel* model() const;
    int count() const;
    int index() const;
    bool ready() const;
    QQmlPropertyMap* propertyMap() const;
    bool keepIndexTrack() const;

public slots:
    void setModel(QAbstractListModel* value);
    void setIndex(int value);
    void setPropertyMap(QQmlPropertyMap* value);
    void setKeepIndexTrack(bool value);

private slots:
    void setReady(bool value);

signals:
    void modelChanged(QAbstractListModel* model);
    void countChanged(int);
    void indexChanged(int index);
    void readyChanged(bool ready);
    void propertyMapChanged(QQmlPropertyMap* propertyMap);
    void keepIndexTrackChanged(bool keepIndexTrack);
// --- ---

private:
    bool isValidIndex() const;
    void reload();
    void reloadRoles(const QVector<int>& affectedRoles = {});

    // From QML
    void onValueChanged(const QString &key, const QVariant &value);

    // From model
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onRowsInsertedBefore(const QModelIndex& parent, int first, int last);
    void onRowsRemoved(const QModelIndex& parent, int first, int last);
    void onRowsRemovedBefore(const QModelIndex& parent, int first, int last);
    void onModelReset();
    void onRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int> &roles);

private:
    DECLARE_PIMPL
};
