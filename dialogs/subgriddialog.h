#ifndef SUBGRIDDIALOG_H
#define SUBGRIDDIALOG_H

#include <QDialog>

class CartesianGrid;
class IJQuick3DViewer;
class VariableSelector;

namespace Ui {
class SubgridDialog;
}

class SubgridDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubgridDialog( CartesianGrid* cg, QWidget *parent = nullptr);
    ~SubgridDialog();

private:
    Ui::SubgridDialog *ui;
    CartesianGrid* m_cg;
    IJQuick3DViewer* m_previewWidget;
    VariableSelector* m_variableForPreview;

private Q_SLOTS:

    void onSave();
    void onPreview();
};

#endif // SUBGRIDDIALOG_H
