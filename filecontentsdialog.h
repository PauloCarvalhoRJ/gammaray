#ifndef FILECONTENTSDIALOG_H
#define FILECONTENTSDIALOG_H

#include <QDialog>

namespace Ui {
class FileContentsDialog;
}

class FileContentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileContentsDialog(QWidget *parent = 0, const QString file_path = "", const QString title = "");
    ~FileContentsDialog();

private:
    Ui::FileContentsDialog *ui;
};

#endif // FILECONTENTSDIALOG_H
