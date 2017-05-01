#ifndef CREATEGRIDDIALOG_H
#define CREATEGRIDDIALOG_H

#include <QDialog>

class PointSet;
class VariogramModelList;
class WidgetGSLibParGrid;
class GSLibParGrid;

namespace Ui {
class CreateGridDialog;
}

class CreateGridDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateGridDialog(PointSet* pointSet, QWidget *parent = 0);
    ~CreateGridDialog();

private:
    Ui::CreateGridDialog *ui;
    PointSet* m_pointSet;
    VariogramModelList* m_vModelList;
    WidgetGSLibParGrid* m_gridParameters;
    GSLibParGrid* m_par;

private slots:
    void runGammaBar();
    void createGridAndClose();
    void calcN();
    void preview();
    void onVariogramClicked();
};

#endif // CREATEGRIDDIALOG_H
