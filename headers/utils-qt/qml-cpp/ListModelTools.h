#pragma once
#include <QAbstractListModel>
#include <utils-cpp/pimpl.h>

class ListModelTools : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QAbstractListModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int itemsCount READ itemsCount /*WRITE setItemsCount*/ NOTIFY itemsCountChanged)

    static void registerTypes(const char* url);

    explicit ListModelTools(QObject* parent = nullptr);
    ~ListModelTools() override;

// --- Properties support ---
public:
    QAbstractListModel* model() const;
    int itemsCount() const;
public slots:
    void setModel(QAbstractListModel* value);
    void setItemsCount(int value);
signals:
    void modelChanged(QAbstractListModel* model);
    void itemsCountChanged(int itemsCount);
// --- ---

private:
    void updateItemsCount();

    // From model
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onRowsRemoved(const QModelIndex& parent, int first, int last);
    void onModelReset();

private:
    DECLARE_PIMPL
};
