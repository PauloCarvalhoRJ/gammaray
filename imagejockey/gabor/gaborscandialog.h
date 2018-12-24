#ifndef GABORSCANDIALOG_H
#define GABORSCANDIALOG_H

#include <QDialog>

namespace Ui {
class GaborScanDialog;
}

class GaborScanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GaborScanDialog(QWidget *parent = 0);
    ~GaborScanDialog();

private:
    Ui::GaborScanDialog *ui;
};

#endif // GABORSCANDIALOG_H
