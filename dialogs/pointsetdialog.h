#ifndef POINTSETDIALOG_H
#define POINTSETDIALOG_H

#include <QDialog>

namespace Ui {
class PointSetDialog;
}

class PointSetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PointSetDialog(QWidget *parent = 0, const QString file_path = "");
    ~PointSetDialog();

public slots:
    //indexes in GSLib format start with 1, not 0 (zero).
    int getXFieldIndex();
    int getYFieldIndex();
    int getZFieldIndex();
    QString getNoDataValue();

private:
    Ui::PointSetDialog *ui;
    QString _file_path;
};

#endif // POINTSETDIALOG_H
