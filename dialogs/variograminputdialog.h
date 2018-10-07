#ifndef VARIOGRAMINPUTDIALOG_H
#define VARIOGRAMINPUTDIALOG_H

#include <QDialog>

class VariogramModelList;
class VariogramModel;

namespace Ui {
class VariogramInputDialog;
}

class VariogramInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VariogramInputDialog(QWidget *parent = 0);
    ~VariogramInputDialog();

    VariogramModel* getSelectedVariogramModel();

    void setCaption( QString caption );

private:
    Ui::VariogramInputDialog *ui;
    VariogramModelList* m_variogramList;
};

#endif // VARIOGRAMINPUTDIALOG_H
