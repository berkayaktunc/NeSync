#include "DataModels/TabFileExplorer/TableModelFileExplorer.h"
#include "DataModels/TabFileExplorer/ListModelFileExplorer.h"
#include "ui_TabFileExplorer.h"

#include "TabFileExplorer.h"
#include "Utility/JsonDtoFormat.h"
#include "Backend/FileStorageSubSystem/FileStorageManager.h"

#include <QQueue>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QtConcurrent>
#include <QStandardPaths>
#include <QProgressDialog>

TabFileExplorer::TabFileExplorer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabFileExplorer)
{
    ui->setupUi(this);

    dialogEditVersion = new DialogEditVersion(this);
    dialogCreateCopy = new DialogCreateCopy(this);
    dialogExport = new DialogExport(this);

    buildContextMenuTableFileExplorer();
    buildContextMenuListFileExplorer();

    ui->buttonBack->setDisabled(true);
    ui->buttonForward->setDisabled(true);
    clearDescriptionDetails();

    this->ui->tableViewFileExplorer->horizontalHeader()->setMinimumSectionSize(110);
    this->ui->tableViewFileExplorer->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);

    ui->lineEditWorkingDir->setText(FileStorageManager::separator);
    displayFolderInTableViewFileExplorer(FileStorageManager::separator);
}

TabFileExplorer::~TabFileExplorer()
{
    delete ui;
}

void TabFileExplorer::buildContextMenuTableFileExplorer()
{
    contextMenuTableFileExplorer = new QMenu(ui->tableViewFileExplorer);
    QMenu *ptrMenu = contextMenuTableFileExplorer;

    QAction *actionExport = ui->contextActionTableFileExplorer_Export;
    QAction *actionFreeze = ui->contextActionTableFileExplorer_Freeze;
    QAction *actionDelete = ui->contextActionTableFileExplorer_Delete;

    ptrMenu->addAction(actionExport);
    ptrMenu->addAction(actionFreeze);
    ptrMenu->addAction(actionDelete);

    QObject::connect(ui->tableViewFileExplorer, &QTableView::customContextMenuRequested,
                     this, &TabFileExplorer::showContextMenuTableView);
}

void TabFileExplorer::buildContextMenuListFileExplorer()
{
    contextMenuListFileExplorer = new QMenu(ui->listView);
    QMenu *ptrMenu = contextMenuListFileExplorer;

    QAction *actionEditVersion = ui->contextActionListFileExplorer_EditVersion;
    QAction *actionCreateCopy = ui->contextActionListFileExplorer_CreateCopy;
    QAction *actionSetAsCurrentVerion = ui->contextActionListFileExplorer_SetAsCurrentVersion;
    QAction *actionDelete = ui->contextActionListFileExplorer_DeleteVersion;

    ptrMenu->addAction(actionEditVersion);
    ptrMenu->addAction(actionCreateCopy);
    ptrMenu->addAction(actionSetAsCurrentVerion);
    ptrMenu->addAction(actionDelete);

    QObject::connect(ui->listView, &QListView::customContextMenuRequested,
                     this, &TabFileExplorer::showContextMenuListView);
}

void TabFileExplorer::createNavigationHistoryIndex(const QString &path)
{
    auto tokenList = path.split(FileStorageManager::separator);
    tokenList.removeLast();

    for(QString &token : tokenList)
        token.append(FileStorageManager::separator);

    QString aggregator;
    int index = 0;

    navigationHistoryIndices.clear();

    for(QString &token : tokenList)
    {
        aggregator.append(token);
        navigationHistoryIndices.append(aggregator);
    }
}

void TabFileExplorer::displayFolderInTableViewFileExplorer(const QString &symbolFolderPath)
{
    auto fsm = FileStorageManager::instance();
    QJsonObject folderJson = fsm->getFolderJsonBySymbolPath(symbolFolderPath, true);
    QTableView *tableView = ui->tableViewFileExplorer;

    if(tableView->model() != nullptr)
        delete tableView->model();

    auto tableModel = new TableModelFileExplorer(folderJson, this);
    tableView->setModel(tableModel);

    ui->lineEditWorkingDir->setText(folderJson[JsonKeys::Folder::SymbolFolderPath].toString());

    tableView->hideColumn(TableModelFileExplorer::ColumnIndexSymbolPath);
    tableView->hideColumn(TableModelFileExplorer::ColumnIndexItemType);
    tableView->viewport()->update();
    tableView->resizeColumnsToContents();
}

// https://stackoverflow.com/a/73912542
// https://stackoverflow.com/a/566085
// https://stackoverflow.com/questions/565704/how-to-correctly-convert-filesize-in-bytes-into-mega-or-gigabytes
QString TabFileExplorer::fileSizeToString(qulonglong fileSize) const
{
    QString result = "";

    qulonglong tb = Q_UINT64_C(1000 * 1000 * 1000 * 1000);
    qulonglong gb = Q_UINT64_C(1000 * 1000 * 1000);
    qulonglong mb = Q_UINT64_C(1000 * 1000);
    qulonglong kb = Q_UINT64_C(1000);

    double fileSizeDouble = (double) fileSize;

    if(fileSize >= tb)
        result = QString::number((fileSizeDouble/tb), 'g', 3) + " TB";

    else if(fileSize >= gb)
        result = QString::number((fileSizeDouble/gb), 'g', 3) + " GB";

    else if(fileSize >= mb)
        result = QString::number((fileSizeDouble/mb), 'g', 3) + " MB";

    else if(fileSize >= kb)
        result = QString::number((fileSizeDouble/kb), 'g', 4) + " KB";

    else
        result = QString::number(fileSize) + " bytes";

    return result;
}

QString TabFileExplorer::currentSymbolFolderPath() const
{
    return ui->lineEditWorkingDir->text();
}

void TabFileExplorer::refreshFileExplorer()
{
    displayFolderInTableViewFileExplorer(ui->lineEditWorkingDir->text());
}

void TabFileExplorer::showContextMenuTableView(const QPoint &argPos)
{
    QAbstractItemView *subjectView = ui->tableViewFileExplorer;
    QModelIndex index = subjectView->indexAt(argPos);

    if(index.isValid()) // If user selected an item from table.
    {
        auto tableModel = (TableModelFileExplorer *) subjectView->model();
        bool isFrozen = tableModel->getIsFrozenFromModelIndex(index);

        if(isFrozen)
            ui->contextActionTableFileExplorer_Freeze->setText(tr("Thaw"));
        else
            ui->contextActionTableFileExplorer_Freeze->setText(tr("Freeze"));

        QMenu *ptrMenu = contextMenuTableFileExplorer;
        ptrMenu->popup(subjectView->viewport()->mapToGlobal(argPos));
    }
}

void TabFileExplorer::showContextMenuListView(const QPoint &argPos)
{
    QAbstractItemView *subjectView = ui->listView;
    QModelIndex index = subjectView->indexAt(argPos);

    if(index.isValid()) // If user selected an item from list.
    {
        auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();
        QModelIndex tableModelIndex = ui->tableViewFileExplorer->selectionModel()->selectedRows().first();
        bool isFrozen = tableModel->getIsFrozenFromModelIndex(tableModelIndex);

        if(isFrozen)
            ui->contextActionListFileExplorer_SetAsCurrentVersion->setEnabled(false);
        else
            ui->contextActionListFileExplorer_SetAsCurrentVersion->setEnabled(true);

        QMenu *ptrMenu = contextMenuListFileExplorer;
        ptrMenu->popup(subjectView->viewport()->mapToGlobal(argPos));
    }
}

void TabFileExplorer::on_tableViewFileExplorer_clicked(const QModelIndex &index)
{
    if(index.isValid()) // If user clicked an item
    {
        clearDescriptionDetails();

        auto tableModel = (TableModelFileExplorer *)index.model();
        QString symbolPath = tableModel->getSymbolPathFromModelIndex(index);
        TableModelFileExplorer::TableItemType type = tableModel->getItemTypeFromModelIndex(index);

        if(type == TableModelFileExplorer::TableItemType::File)
        {
            ListModelFileExplorer *listModel = nullptr;
            if(ui->listView->model() != nullptr)
                delete ui->listView->model();

            auto fsm = FileStorageManager::instance();
            QJsonObject fileJson = fsm->getFileJsonBySymbolPath(symbolPath);

            listModel = new ListModelFileExplorer(fileJson, ui->listView);
            ui->listView->setModel(listModel);

            qlonglong latestRow = fileJson[JsonKeys::File::MaxVersionNumber].toInt() - 1;
            ui->listView->setCurrentIndex(listModel->index(latestRow, 0));
            on_listView_clicked(listModel->index(latestRow, 0));
        }
    }
}

void TabFileExplorer::on_tableViewFileExplorer_doubleClicked(const QModelIndex &index)
{
    if(index.isValid()) // If user double clicked an item
    {
        auto model = (TableModelFileExplorer *)index.model();
        QString symbolPath = model->getSymbolPathFromModelIndex(index);
        TableModelFileExplorer::TableItemType type = model->getItemTypeFromModelIndex(index);

        if(type == TableModelFileExplorer::TableItemType::Folder)
        {
            createNavigationHistoryIndex(symbolPath);
            ui->buttonForward->setDisabled(true);

            // Enable back button whenever item is double clicked.
            ui->buttonBack->setEnabled(true);

            clearDescriptionDetails();
            displayFolderInTableViewFileExplorer(symbolPath);
        }
    }
}

void TabFileExplorer::on_listView_clicked(const QModelIndex &index)
{
    if(index.isValid())
    {
        auto model = (ListModelFileExplorer *) ui->listView->model();
        qlonglong versionNumber = model->data(index, Qt::ItemDataRole::DisplayRole).toLongLong();
        auto fsm = FileStorageManager::instance();
        QJsonObject fileVersionJson = fsm->getFileVersionJson(model->getFileSymbolPath(), versionNumber);

        QString size = fileSizeToString(fileVersionJson[JsonKeys::FileVersion::Size].toInteger());
        ui->labelDataSize->setText(size);
        ui->labelDataDate->setText(fileVersionJson[JsonKeys::FileVersion::Timestamp].toString());
        ui->textEditDescription->setText(fileVersionJson[JsonKeys::FileVersion::Description].toString());
    }
}

void TabFileExplorer::on_buttonBack_clicked()
{
    clearDescriptionDetails();

    if(ui->lineEditWorkingDir->text() != navigationHistoryIndices.first())
    {
        auto currentIndex = navigationHistoryIndices.indexOf(ui->lineEditWorkingDir->text());
        auto newIndex = currentIndex - 1;

        if(newIndex == 0)
            ui->buttonBack->setDisabled(true);

        ui->buttonForward->setEnabled(true);
        displayFolderInTableViewFileExplorer(navigationHistoryIndices.at(newIndex));
    }
}

void TabFileExplorer::on_buttonForward_clicked()
{
    clearDescriptionDetails();

    if(ui->lineEditWorkingDir->text() != navigationHistoryIndices.last())
    {
        auto currentIndex = navigationHistoryIndices.indexOf(ui->lineEditWorkingDir->text());
        auto newIndex = currentIndex + 1;

        if(newIndex == navigationHistoryIndices.size() - 1)
            ui->buttonForward->setDisabled(true);

        ui->buttonBack->setEnabled(true);
        displayFolderInTableViewFileExplorer(navigationHistoryIndices.at(newIndex));
    }
}

void TabFileExplorer::clearDescriptionDetails()
{
    ui->labelDataSize->setText("-");
    ui->labelDataDate->setText("-");
    ui->textEditDescription->setText("");

    QAbstractItemModel *listModel = ui->listView->model();

    if(listModel != nullptr)
        delete listModel;
}

void TabFileExplorer::on_contextActionListFileExplorer_EditVersion_triggered()
{
    QItemSelectionModel *tableViewSelectionModel = ui->tableViewFileExplorer->selectionModel();
    QModelIndexList listSymbolPath = tableViewSelectionModel->selectedRows(TableModelFileExplorer::ColumnIndexSymbolPath);
    auto fileSymbolPath = listSymbolPath.first().data().toString();

    QItemSelectionModel * listViewSelectionModel = ui->listView->selectionModel();
    QModelIndexList listVersionNumber = listViewSelectionModel->selectedRows(ListModelFileExplorer::ColumnIndexVersionNumber);
    auto versionNumber = listVersionNumber.first().data().toLongLong();

    dialogEditVersion->setModal(true);
    dialogEditVersion->show(fileSymbolPath, versionNumber);
}

void TabFileExplorer::on_contextActionListFileExplorer_DeleteVersion_triggered()
{
    auto fsm = FileStorageManager::instance();

    QModelIndex tableModelIndex = ui->tableViewFileExplorer->selectionModel()->selectedRows().first();
    auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();
    QString symbolFilePath = tableModel->getSymbolPathFromModelIndex(tableModelIndex);
    QString userFilePath = tableModel->getUserPathFromModelIndex(tableModelIndex);
    QString fileName = tableModel->getNameFromModelIndex(tableModelIndex);

    QJsonObject fileJson = fsm->getFileJsonBySymbolPath(symbolFilePath);

    if(fileJson[JsonKeys::File::MaxVersionNumber].toInteger() == 1)
    {
        QString title = tr("Can't delete single version of the file !");
        QString message = tr("File <b>%1</b> has only one version, deleting this version means deleting the file. To do that, delete the file.");
        message = message.arg(fileName);
        QMessageBox::information(this, title, message);
        return;
    }

    qlonglong selectedVersionNumber = ui->listView->selectionModel()->selectedRows().first().data().toLongLong();

    QString title = tr("Delete file version ?");
    QString message = tr("Do you want to delete version <b>%1</b> of file <b>%2</b>.");
    message = message.arg(selectedVersionNumber).arg(fileName);
    QMessageBox::StandardButton result = QMessageBox::question(this, title, message);

    if(result == QMessageBox::StandardButton::Yes)
    {
        QFutureWatcher<void> futureWatcher;
        QProgressDialog dialog(this);
        dialog.setLabelText(tr("Deleting version <b>%1</b> of file <b>%2</b>...").arg(selectedVersionNumber).arg(fileName));
        dialog.setCancelButton(nullptr);

        QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
        QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
        QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

        emit signalStopFileMonitor();

        QFuture<void> future = QtConcurrent::run([=, &fileJson]{
            qlonglong recentMaxVersion = fileJson[JsonKeys::File::MaxVersionNumber].toInteger();
            fsm->deleteFileVersion(symbolFilePath, selectedVersionNumber);

            if(recentMaxVersion == selectedVersionNumber) // If current version is deleted
            {
                fileJson = fsm->getFileJsonBySymbolPath(symbolFilePath);
                QJsonObject versionJson = fsm->getFileVersionJson(symbolFilePath, fileJson[JsonKeys::File::MaxVersionNumber].toInteger());
                QString internalFilePath = fsm->getBackupFolderPath() + versionJson[JsonKeys::FileVersion::InternalFileName].toString();
                QFile::remove(userFilePath);
                QFile::copy(internalFilePath, userFilePath);

                emit signalStopMonitoringItem(userFilePath);
                emit signalStartMonitoringItem(userFilePath);
            }
        });

        futureWatcher.setFuture(future);
        dialog.exec();
        futureWatcher.waitForFinished();

        emit signalStartFileMonitor();
        refreshFileExplorer();
    }
}

void TabFileExplorer::on_contextActionListFileExplorer_CreateCopy_triggered()
{
    auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();
    QModelIndex tableModelIndex = ui->tableViewFileExplorer->selectionModel()->selectedRows().first();
    QString symbolFilePath = tableModel->getSymbolPathFromModelIndex(tableModelIndex);
    qlonglong selectedVersionNumber = ui->listView->selectionModel()->selectedRows().first().data().toLongLong();

    dialogCreateCopy->setModal(true);
    dialogCreateCopy->show(symbolFilePath, selectedVersionNumber);
}

void TabFileExplorer::on_contextActionListFileExplorer_SetAsCurrentVersion_triggered()
{
    auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();
    QModelIndex tableModelIndex = ui->tableViewFileExplorer->selectionModel()->selectedRows().first();
    QString symbolFilePath = tableModel->getSymbolPathFromModelIndex(tableModelIndex);
    QString userFilePath = tableModel->getUserPathFromModelIndex(tableModelIndex);
    QString fileName = tableModel->getNameFromModelIndex(tableModelIndex);
    qlonglong selectedVersionNumber = ui->listView->selectionModel()->selectedRows().first().data().toLongLong();

    QString title = tr("Change current version ?");
    QString message = tr("Version <b>%1</b> of file <b>%2</b> will be replaced in filesystem"
                         " and that version will be the monitored file. Would you like to do that ?");
    message = message.arg(selectedVersionNumber).arg(fileName);

    QMessageBox::StandardButton result = QMessageBox::question(this, title, message);

    if(result == QMessageBox::StandardButton::Yes)
    {
        QFutureWatcher<void> futureWatcher;
        QProgressDialog dialog(this);
        dialog.setLabelText(tr("Changing current version of file <b>%1</b>...").arg(fileName));

        QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
        QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
        QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

        emit signalStopFileMonitor();
        emit signalStopMonitoringItem(userFilePath);

        QFuture<void> future = QtConcurrent::run([=]{
            QFile::remove(userFilePath);

            auto fsm = FileStorageManager::instance();
            QJsonObject fileJson = fsm->getFileJsonBySymbolPath(symbolFilePath);
            QJsonObject versionJson = fsm->getFileVersionJson(symbolFilePath, selectedVersionNumber);
            qlonglong newVersionNumber = fileJson[JsonKeys::File::MaxVersionNumber].toInteger() + 1;
            versionJson[JsonKeys::FileVersion::NewVersionNumber] = newVersionNumber;

            fsm->updateFileVersionEntity(versionJson);
            fsm->sortFileVersionsInIncreasingOrder(symbolFilePath);

            QString internalFilePath = fsm->getBackupFolderPath();
            internalFilePath += versionJson[JsonKeys::FileVersion::InternalFileName].toString();

            QFile::copy(internalFilePath, userFilePath);
        });

        futureWatcher.setFuture(future);
        dialog.exec();
        futureWatcher.waitForFinished();

        emit signalStartMonitoringItem(userFilePath);
        emit signalStartFileMonitor();

        refreshFileExplorer();
    }
}

void TabFileExplorer::on_contextActionTableFileExplorer_Export_triggered()
{
    QList<QString> selectedItemList;
    QModelIndexList list = ui->tableViewFileExplorer->selectionModel()->selectedRows();
    auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();

    for(const QModelIndex &currentIndex : list)
    {
        QString symbolPath = tableModel->getSymbolPathFromModelIndex(currentIndex);
        selectedItemList.append(symbolPath);
    }

    dialogExport->setModal(true);
    dialogExport->show(selectedItemList);
}

void TabFileExplorer::on_contextActionTableFileExplorer_Delete_triggered()
{
    auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();
    QModelIndex modelIndex = ui->tableViewFileExplorer->selectionModel()->selectedRows().first();

    TableModelFileExplorer::TableItemType type = tableModel->getItemTypeFromModelIndex(modelIndex);
    QString name = tableModel->getNameFromModelIndex(modelIndex);
    QString symbolPath = tableModel->getSymbolPathFromModelIndex(modelIndex);
    QString userPath = tableModel->getUserPathFromModelIndex(modelIndex);
    bool isFrozen = tableModel->getIsFrozenFromModelIndex(modelIndex);

    QString title = tr("Delete item ?");
    QString message;

    if(type == TableModelFileExplorer::TableItemType::Folder)
        message = tr("Deleting folder <b>%1</b> will remove everything inside it including the folder itself.");
    else if(type == TableModelFileExplorer::TableItemType::File)
        message = tr("Deleting file <b>%1</b> will remove all versions of it.");

    message = message.arg(name);

    QMessageBox::StandardButton result = QMessageBox::question(this, title, message);

    if(result == QMessageBox::StandardButton::Yes)
    {
        auto fsm = FileStorageManager::instance();
        QFutureWatcher<void> futureWatcher;
        QProgressDialog dialog(this);
        dialog.setCancelButton(nullptr);

        QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
        QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
        QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

        if(type == TableModelFileExplorer::TableItemType::Folder)
        {
            dialog.setLabelText(tr("Deleting folder <b>%1</b>...").arg(name));

            QFuture<void> future = QtConcurrent::run([=]{
                fsm->deleteFolder(symbolPath);

                if(!isFrozen)
                    emit signalStopMonitoringItem(userPath);
            });

            futureWatcher.setFuture(future);
            dialog.exec();
            futureWatcher.waitForFinished();
        }
        else if(type == TableModelFileExplorer::TableItemType::File)
        {
            dialog.setLabelText(tr("Deleting file <b>%1</b>...").arg(name));

            QFuture<void> future = QtConcurrent::run([=]{
                fsm->deleteFile(symbolPath);

                if(!isFrozen)
                    emit signalStopMonitoringItem(userPath);
            });

            futureWatcher.setFuture(future);
            dialog.exec();
            futureWatcher.waitForFinished();
        }

        refreshFileExplorer();
        clearDescriptionDetails();
    }
}

void TabFileExplorer::on_contextActionTableFileExplorer_Freeze_triggered()
{
    auto fsm = FileStorageManager::instance();
    auto tableModel = (TableModelFileExplorer *) ui->tableViewFileExplorer->model();
    QModelIndex modelIndex = ui->tableViewFileExplorer->selectionModel()->selectedRows().first();

    TableModelFileExplorer::TableItemType type = tableModel->getItemTypeFromModelIndex(modelIndex);
    QString name = tableModel->getNameFromModelIndex(modelIndex);
    QString symbolPath = tableModel->getSymbolPathFromModelIndex(modelIndex);
    QString userPath = tableModel->getUserPathFromModelIndex(modelIndex);
    bool isFrozen = tableModel->getIsFrozenFromModelIndex(modelIndex);

    if(type == TableModelFileExplorer::TableItemType::Folder)
        executeFreezingOrThawingOfFolder(name, symbolPath, userPath, isFrozen);
    else if(type == TableModelFileExplorer::TableItemType::File)
        executeFreezingOrThawingOfFile(name, symbolPath, userPath, isFrozen);

    refreshFileExplorer();
}

void TabFileExplorer::executeFreezingOrThawingOfFolder(const QString &name, const QString &symbolPath, const QString &userPath, bool isFrozen)
{
    auto fsm = FileStorageManager::instance();
    QJsonObject folderJson = fsm->getFolderJsonBySymbolPath(symbolPath);

    if(!isFrozen) // If freezing folder
    {
        folderJson[JsonKeys::Folder::IsFrozen] = true;
        bool isUpdated = fsm->updateFolderEntity(folderJson, true);

        if(isUpdated)
            emit signalStopMonitoringItem(userPath);
    }
    else // If thawing folder
    {
        auto parentSymbolPath = folderJson[JsonKeys::Folder::ParentFolderPath].toString();
        QJsonObject parentFolderJson = fsm->getFolderJsonBySymbolPath(parentSymbolPath);

        if(parentFolderJson[JsonKeys::Folder::IsFrozen].toBool())
        {
            QString title = tr("Parent folder is frozen !");
            QString message = tr("Can't thaw child folder because parent is frozen. To thaw <b>%1</b>, thaw the parent first.");
            message = message.arg(name);
            QMessageBox::information(this, title, message);
            return;
        }

        QString title = tr("Select location for thawed folder");
        QString message = tr("Thawed folder <b>%1</b> and all content of it will be placed to location you'll select now.");
        message = message.arg(name);
        QMessageBox::information(this, title, message);

        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation);
        QString selection = QFileDialog::getExistingDirectory(this,
                                                              tr("Select location for folder to be thawed"),
                                                              desktopPath,
                                                              QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

        selection = QDir::toNativeSeparators(selection).append(QDir::separator());

        bool isExist = QDir(selection + name).exists();
        if(isExist)
        {
            QString title = tr("Folder exist at location !");
            QString message = tr("Can't overwrite folder <b>%1</b> in location you selected.");
            message = message.arg(name);
            QMessageBox::critical(this, title, message);
            return;
        }

        // TODO add checking empty space before extracting folders

        emit signalStopFileMonitor();
        thawFolderTree(name, symbolPath, selection);
        emit signalStartFileMonitor();
    }
}

void TabFileExplorer::executeFreezingOrThawingOfFile(const QString &name, const QString &symbolPath, const QString &userPath, bool isFrozen)
{
    auto fsm = FileStorageManager::instance();
    QJsonObject fileJson = fsm->getFileJsonBySymbolPath(symbolPath);
    QJsonObject parentFolderJson = fsm->getFolderJsonBySymbolPath(fileJson[JsonKeys::File::SymbolFolderPath].toString(),
                                                                  fileJson[JsonKeys::File::MaxVersionNumber].toInteger());

    if(!isFrozen) // If freezing file
    {
        fileJson[JsonKeys::File::IsFrozen] = true;
        bool isUpdated = fsm->updateFileEntity(fileJson);
        if(isUpdated)
            emit signalStopMonitoringItem(userPath);
    }
    else // If thawing folder
    {
        if(parentFolderJson[JsonKeys::Folder::IsFrozen].toBool())
        {
            QString title = tr("Parent folder is frozen !");
            QString message = tr("Can't thaw child file because parent folder is frozen. To thaw <b>%1</b>, thaw the parent folder first.");
            message = message.arg(name);
            QMessageBox::information(this, title, message);
            return;
        }

        QJsonObject versionJson = fsm->getFileVersionJson(symbolPath, fileJson[JsonKeys::File::MaxVersionNumber].toInteger());
        QString userFolderPath = parentFolderJson[JsonKeys::Folder::UserFolderPath].toString();
        QString userFilePath = userFolderPath + name;
        QString internalFilePath = fsm->getBackupFolderPath() + versionJson[JsonKeys::FileVersion::InternalFileName].toString();

        bool isExist = QFile::exists(userFilePath);
        if(isExist)
        {
            QString title = tr("File exist at location !");
            QString message = tr("Can't overwrite file <b>%1</b> in location you selected.");
            message = message.arg(name);
            QMessageBox::critical(this, title, message);
            return;
        }

        emit signalStopFileMonitor();
        QFutureWatcher<void> futureWatcher;
        QProgressDialog dialog(this);
        dialog.setLabelText(tr("Thawing file <b>%1</b>...").arg(name));

        QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
        QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
        QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

        bool isCopied = false;

        QFuture<void> future = QtConcurrent::run([=, &isCopied] {
            isCopied = QFile::copy(internalFilePath, userFilePath);
        });

        futureWatcher.setFuture(future);
        dialog.exec();
        futureWatcher.waitForFinished();

        if(isCopied)
        {
            fileJson[JsonKeys::File::IsFrozen] = false;
            bool isUpdated = fsm->updateFileEntity(fileJson);
            if(isUpdated)
                emit signalStartMonitoringItem(userFilePath);
        }
        emit signalStartFileMonitor();
    }
}

void TabFileExplorer::thawFolderTree(const QString folderName, const QString &parentSymbolFolderPath, const QString &targetUserPath)
{
    QString nativePath = QDir::toNativeSeparators(targetUserPath);
    if(!nativePath.endsWith(QDir::separator()))
        nativePath.append(QDir::separator());

    QFutureWatcher<void> futureWatcher;
    QProgressDialog dialog(this);
    dialog.setLabelText(tr("Thawing folder <b>%1</b>...").arg(folderName));

    QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
    QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
    QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

    QFuture<void> future = QtConcurrent::run([=] {

        QString currentUserPath = nativePath;
        auto fsm = FileStorageManager::instance();
        QJsonObject parentFolderJson = fsm->getFolderJsonBySymbolPath(parentSymbolFolderPath, true);

        QQueue<QJsonObject> symbolFolders;
        symbolFolders.enqueue(parentFolderJson);

        while(!symbolFolders.empty())
        {
            QJsonObject currentFolder = symbolFolders.dequeue();
            auto suffixPath = currentFolder[JsonKeys::Folder::SuffixPath].toString().chopped(1);
            currentUserPath.append(suffixPath).append(QDir::separator());

            QDir currentDir(currentUserPath);
            bool isCreated = currentDir.mkpath(currentUserPath);

            if(isCreated)
            {
                currentFolder[JsonKeys::Folder::UserFolderPath] = currentUserPath;
                currentFolder[JsonKeys::Folder::IsFrozen] = false;
                bool isUpdated = fsm->updateFolderEntity(currentFolder, true);
                if(isUpdated)
                    emit signalStartMonitoringItem(currentUserPath); // Notify about created folder

                QJsonArray childFiles = currentFolder[JsonKeys::Folder::ChildFiles].toArray();
                QJsonArray childFolders = currentFolder[JsonKeys::Folder::ChildFolders].toArray();

                for(const QJsonValue &currentChildFile : childFiles)
                {
                    QJsonObject fileJson = currentChildFile.toObject();
                    QJsonObject versionJson = fsm->getFileVersionJson(fileJson[JsonKeys::File::SymbolFilePath].toString(),
                                                                      fileJson[JsonKeys::File::MaxVersionNumber].toInteger());

                    QString internalFilePath = fsm->getBackupFolderPath();
                    internalFilePath.append(versionJson[JsonKeys::FileVersion::InternalFileName].toString());
                    QString userFilePath = currentUserPath + fileJson[JsonKeys::File::FileName].toString();

                    bool isCopied = QFile::copy(internalFilePath, userFilePath);
                    if(isCopied)
                        emit signalStartMonitoringItem(userFilePath); // Notify about copied file
                }

                for(const QJsonValue &currentChildFolder : childFolders)
                {
                    QString childFolderPath = currentChildFolder.toObject()[JsonKeys::Folder::SymbolFolderPath].toString();
                    symbolFolders.enqueue(fsm->getFolderJsonBySymbolPath(childFolderPath, true));
                }
            }
        }
    });

    futureWatcher.setFuture(future);
    dialog.exec();
    futureWatcher.waitForFinished();
}
