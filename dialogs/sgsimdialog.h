#ifndef SGSIMDIALOG_H
#define SGSIMDIALOG_H

#include <QDialog>

namespace Ui {
class SGSIMDialog;
}

class GSLibParGrid;
class WidgetGSLibParGrid;

class SGSIMDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SGSIMDialog(QWidget *parent = 0);
    ~SGSIMDialog();

private:
    Ui::SGSIMDialog *ui;
    GSLibParGrid* m_par;
    WidgetGSLibParGrid* m_gridParameters;
};

#endif // SGSIMDIALOG_H
