#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <QDialog>

namespace Ui {
class SetupDialog;
}

class SetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetupDialog(QWidget *parent = 0);
    ~SetupDialog();

public slots:
    void showGSLibPathSearch();
    void showGSPathSearch();
    void showGVPathSearch();
    void accept();

private:
    Ui::SetupDialog *ui;
};

#endif // SETUPDIALOG_H
