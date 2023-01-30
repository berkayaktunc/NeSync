#ifndef TREEMODELFOLDERMONITOR_H
#define TREEMODELFOLDERMONITOR_H

#include "TreeItem.h"

#include "Backend/FileMonitorSubSystem/FileSystemEventDb.h"

#include <QAbstractItemModel>

class TreeModelFsMonitor : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TreeModelFsMonitor(QObject *parent = nullptr);
    ~TreeModelFsMonitor();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    void setupModelData();
    TreeItem *createTreeItemForFolder(const QString &pathToFolder, TreeItem *root) const;
    QString itemStatusToString(FileSystemEventDb::ItemStatus status) const;

    TreeItem *treeRoot;
    FileSystemEventDb *fsEventDb;

};

#endif // TREEMODELFOLDERMONITOR_H
