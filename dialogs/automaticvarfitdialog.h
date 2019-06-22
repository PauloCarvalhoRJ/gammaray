#ifndef AUTOMATICVARFITDIALOG_H
#define AUTOMATICVARFITDIALOG_H

#include <QDialog>

class Attribute;
class IJGridViewerWidget;

namespace Ui {
class AutomaticVarFitDialog;
}

class AutomaticVarFitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticVarFitDialog( Attribute* at, QWidget *parent = nullptr);
    ~AutomaticVarFitDialog();

private:
    Ui::AutomaticVarFitDialog *ui;

    Attribute* m_at;

    IJGridViewerWidget* m_gridViewerInput;
    IJGridViewerWidget* m_gridViewerVarmap;
};

#endif // AUTOMATICVARFITDIALOG_H
