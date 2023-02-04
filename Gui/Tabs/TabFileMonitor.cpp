#include "TabFileMonitor.h"
#include "ui_TabFileMonitor.h"

#include "DataModels/TabFileMonitor/TreeModelFsMonitor.h"

TabFileMonitor::TabFileMonitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabFileMonitor)
{
    ui->setupUi(this);
    itemDelegateAction = new ItemDelegateAction(this);
    itemDelegateDescription = new ItemDelegateDescription(this);
}

TabFileMonitor::~TabFileMonitor()
{
    delete ui;
}

void TabFileMonitor::onEventDbUpdated()
{
    TreeModelFsMonitor *treeModel = new TreeModelFsMonitor();
    QAbstractItemModel *oldModel = ui->treeView->model();

    if(oldModel != nullptr)
        delete oldModel;

    ui->treeView->setModel(treeModel);
    ui->comboBoxDescriptionNumber->setModel(treeModel->getDescriptionNumberListModel());

    QHeaderView *header = ui->treeView->header();
    header->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    header->setSectionResizeMode(TreeModelFsMonitor::ColumnIndexUserPath, QHeaderView::ResizeMode::Interactive);
    header->setMinimumSectionSize(130);
    ui->treeView->setColumnWidth(TreeModelFsMonitor::ColumnIndexUserPath, 500);

    ui->treeView->setItemDelegateForColumn(TreeModelFsMonitor::ColumnIndexAction, itemDelegateAction);
    ui->treeView->setItemDelegateForColumn(TreeModelFsMonitor::ColumnIndexDescription, itemDelegateDescription);

    ui->treeView->expandAll();

    ui->treeView->setSelectionMode(QAbstractItemView::SelectionMode::ContiguousSelection);
    ui->treeView->selectAll();
    QModelIndexList selectedIndices = ui->treeView->selectionModel()->selectedIndexes();

    for(const QModelIndex &current : selectedIndices)
    {
        ui->treeView->openPersistentEditor(current.siblingAtColumn(TreeModelFsMonitor::ColumnIndexAction));
        ui->treeView->openPersistentEditor(current.siblingAtColumn(TreeModelFsMonitor::ColumnIndexDescription));
    }

    ui->treeView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    ui->treeView->selectionModel()->clearSelection();
}

void TabFileMonitor::on_buttonAddDescription_clicked()
{
    QTextEdit *textEdit = ui->textEditDescription;
    textEdit->blockSignals(true); // Disable updating descriptionMap in TreeModel

    textEdit->clear();

    auto model = (TreeModelFsMonitor *) ui->treeView->model();
    model->appendDescription();

    int number = model->getMaxDescriptionNumber();
    textEdit->setText(model->getDescription(number));

    int lastIndex = model->getDescriptionNumberListModel()->stringList().size() - 1;
    ui->comboBoxDescriptionNumber->setCurrentIndex(lastIndex);

    textEdit->blockSignals(false);
}

void TabFileMonitor::on_buttonDeleteDescription_clicked()
{
    QTextEdit *textEdit = ui->textEditDescription;
    QComboBox *comboBox = ui->comboBoxDescriptionNumber;
    textEdit->blockSignals(true); // Disable updating descriptionMap in TreeModel

    textEdit->clear();

    auto model = (TreeModelFsMonitor *) ui->treeView->model();
    int number = comboBox->currentText().toInt();
    int previousIndex = comboBox->currentIndex();
    model->deleteDescription(number);

    auto list = model->getDescriptionNumberListModel()->stringList();

    if(list.size() > 1)
    {
        if(previousIndex == list.size())
            comboBox->setCurrentIndex(0);
        else
            comboBox->setCurrentIndex(previousIndex);
    }

    int descIndex = comboBox->currentText().toInt();
    textEdit->setText(model->getDescription(descIndex));

    textEdit->blockSignals(false);
}

void TabFileMonitor::on_textEditDescription_textChanged()
{
    auto model = (TreeModelFsMonitor *) ui->treeView->model();
    int number = ui->comboBoxDescriptionNumber->currentText().toInt();
    model->updateDescription(number, ui->textEditDescription->toPlainText());
}

void TabFileMonitor::on_comboBoxDescriptionNumber_activated(int index)
{
    Q_UNUSED(index);

    QTextEdit *textEdit = ui->textEditDescription;
    textEdit->blockSignals(true); // Disable updating descriptionMap in TreeModel

    textEdit->clear();
    auto model = (TreeModelFsMonitor *) ui->treeView->model();
    int number = ui->comboBoxDescriptionNumber->currentText().toInt();
    textEdit->setText(model->getDescription(number));

    textEdit->blockSignals(false);
}

