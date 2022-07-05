#ifndef DIALOGADDNEWFILE_H
#define DIALOGADDNEWFILE_H

#include <QDialog>

#include "DataModels/DialogAddNewFile/TableModelNewAddedFiles.h"

namespace Ui {
class DialogAddNewFile;
}

class DialogAddNewFile : public QDialog
{
    Q_OBJECT

public:
    explicit DialogAddNewFile(QWidget *parent = nullptr);
    ~DialogAddNewFile();

private slots:
    void on_buttonSelectNewFile_clicked();

private:
    void showStatusWarning(const QString &message);
    void showStatusError(const QString &message);

private:
    Ui::DialogAddNewFile *ui;
    TableModelNewAddedFiles *tableModelNewAddedFiles;
};

#endif // DIALOGADDNEWFILE_H
